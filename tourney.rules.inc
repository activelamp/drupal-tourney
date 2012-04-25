<?php

/**
 * @file
 * Rules for handling what happens after a game is won
 *
 */


/**
 * Implements hook_rules_action_info().
 */
function tourney_rules_action_info() {
  $actions = array(
    'tourney_action_get_game_count' => array(
      'label' => t('Get the number of games won by leader in this match'),
      'group' => t('Tourney Rules'),
      'parameter' => array(
        'match' => array(
          'type' => 'tourney_match',
          'label' => t('The match this game belongs in'),
        ),
      ),
      'provides' => array(
        'number_of_won_games' => array(
          'type' => 'integer',
          'label' => t('Number of games')
        ),
      ),
    ),
    'tourney_action_game_is_won' => array(
      'label' => t('Create a new game OR move the players to the next match'),
      'group' => t('Tourney Rules'),
      'parameter' => array(
        'match' => array(
          'type' => 'tourney_match',
          'label' => t('The match this game belongs in'),
        ),
        'quotient' => array(
          'type' => 'text',
          'label' => t('Quotient of games won divided by total games'),
        ),
      ),
      'provides' => array(
        'action_message' => array(
          'type' => 'text',
          'label' => t('Action successful message'),
        ),
      ),
    ),
    'tourney_action_set_all_matches' => array(
      'label' => t('Set contestant in all matches for Round Robin.'),
      'group' => t('Tourney Rules'),
      'parameter' => array(
        'match' => array(
          'type' => 'tourney_match',
          'label' => t('The match this game belongs in'),
        ),
      ),
      'provides' => array(
        'action_message' => array(
          'type' => 'text',
          'label' => t('Action successful message'),
        ),
      ),
    ),
  );

  return $actions;
}

/**
 * Action function to get the number of won games.
 */
function tourney_action_get_game_count($match) {
  // Get the number of games each contestant has won.
  $winners = tourney_get_winner_counts($match, TRUE);

  return array(
    'number_of_won_games' => max($winners),
  );
}

/**
 * Action to either create a new game, or move the winners to new match
 *
 * @param $match (object)
 *   The match the game belongs to
 * @param $quotient (integer)
 *   The number of games won by leader divided by total games in match.
 */
function tourney_action_game_is_won($match, $quotient) {
  $message = '';
  // Create a new game if enough games have not been won yet.
  if ($quotient >= 2) {
    if ( $match->winner ) {
      $matches = tourney_tournament_build_matches($match->tournament);
      if ( $match->next_match_winner ) {
        $wmatch = tourney_get_match_by_name($matches, $match->next_match_winner);
        $cn = "contestant" . array_pop(explode('-', $match->next_match_winner));
        $wmatch->$cn = FALSE;
        tourney_match_save($wmatch);
      }
      if ( $match->next_match_loser ) {
        $lmatch = tourney_get_match_by_name($matches, $match->next_match_loser);
      }
    }
    // Default action to create a new game.
    $create_new_game = TRUE;

    // Ensure every game has a winner selected
    foreach ($match->games_played as $game_id) {
      $game = tourney_game_load($game_id);
      if (!$game->winner) {
        $message = t('Game updated, but not all games have a winner selected. Action not completed.');
        watchdog('tourney', $message);
        $create_new_game = FALSE;
      }
    }
    if ($create_new_game) {
      $game_count = count($match->games_played);
      $game = new stdClass();
      $game->type = 'game';
      $game->title = 'game-' . ++$game_count;
      tourney_game_save($game);
      // Add the relationship to the game.
      $rid = tourney_create_relation('has_game', 'tourney_match', $match->id, 'tourney_game', $game->id);

      $message = t('A new game has been created: !game', array('!game' => l($game->title, 'tourney/game/' . $game->id)));
    }
  }
  elseif ($match->tournament->format != 'RoundRobin') {
    // Get the winner and loser locations into variables.
    @list($wbracket, $wround, $wmatch, $wcontestant) = explode("_", $match->next_match_winner);
    @list($lbracket, $lround, $lmatch, $lcontestant) = explode("_", $match->next_match_loser);

    _tourney_move_next_round($match, $wbracket, $wround, $wmatch, $wcontestant);
    // If there is a place for losers, move them too
    if (isset($lround)) {
      $lmatch_obj = clone $match;
      _tourney_move_next_round($lmatch_obj, $lbracket, $lround, $lmatch, $lcontestant, 'bracket-top');
    }
    _tourney_set_match_winner($match);

    $message = t('Players moved to next round');
  }
  else {
    _tourney_set_match_winner($match);
  }

  return array(
    'action_message' => $message,
  );
} 

/**
 * Recursive function that sets players in a RoundRobin tournament in all their
 * matches once the first round has been setup.
 *
 * @param $match (object)
 *   The match the game belongs to
 */
function tourney_action_set_all_matches($match) {
  @list($wbracket, $wround, $wmatch, $wcontestant) = explode("_", $match->next_match_winner);
  @list($lbracket, $lround, $lmatch, $lcontestant) = explode("_", $match->next_match_loser);
  
  if ($match->next_match_winner) {
    $new_match1 = _tourney_move_next_round($match, $wbracket, $wround, $wmatch, $wcontestant);
    tourney_action_set_all_matches($new_match1);
  }
  if ($match->next_match_loser) {
    $lmatch_obj = clone $match;
    $new_match2 = _tourney_move_next_round($lmatch_obj, $lbracket, $lround, $lmatch, $lcontestant, 'bracket-top');
    tourney_action_set_all_matches($new_match2);
  }
  $message = 'Players added to all rounds';
  
  return array(
    'action_message' => $message,
  );
}

function _tourney_set_match_winner($match) {
  $winners = tourney_get_winner_counts($match);
  $winner = array_keys($winners, max($winners));
  if ( $winners[$winner[0]] > $match->games / 2 ) {
    list($type, $id) = explode(".", $winner[0]);
    $wobj = entity_load($type, array($id));
    $winner = array_pop($wobj);

    // @todo what do we do for a winner thing here... type.id or name or what? using name for now
    $match->winner = $winner->_entity_name;
  }
  else {
    $match->winner = '';
  }
  tourney_match_save($match);
}

/**
 * Search through all matches in the next round that has a match title that is
 * the same as the path for the next match. Save the match found with the new
 * contestant for that match.
 *
 * @param $match
 *   The match object the game belongs to.
 * @param $bracket
 *   The bracket name as a string to search for the **next match**.
 * @param $round
 *   The round name as a string to search for the **next match**.
 * @param $match_name
 *   The match name as a string to search for the **next match**. This will look for
 *   any matches with this match_name.
 * @param $contestant
 *   The contestant name as a string to search for the **next match**. Not the actual
 *   contestant, just the string identifier (i.e. contestant-1, contestant-2).
 * @param $from
 *   A variable used to distinguish if this is being called for a loser path.
 *   @see tourney_action_game_is_won()
 */
function _tourney_move_next_round($match, $bracket, $round, $match_name, $contestant, $from = 'same') {
  // Get the tournament object this match is in.
  $tournament = relation_get_related_entity('tourney_match', $match->id, 'has_match');

  // Get all the relations in the destination round.
  $query = relation_query('tourney_tournament', $tournament->id);
  $query->entityCondition('bundle', 'has_match')
        ->fieldCondition('round', 'value', $round)
        ->fieldCondition('bracket', 'value', $bracket)
        ->range(0, $tournament->matches_created);
  $results = $query->execute();

  // Find the match the contestant should go to.
  foreach ($results as $relation) {
    $r = relation_load($relation->rid);
    $next_match = $r->endpoints[LANGUAGE_NONE][1]['entity_type'] == 'tourney_match'
      ? tourney_match_load($r->endpoints[LANGUAGE_NONE][1]['entity_id'])
      : NULL;

    // We have a match, lets do something with it.
    if ($next_match->title == $match_name && empty($next_match->$contestant)) {
      $contestant = str_replace('-', '', $contestant);
    
      $next_match->$contestant = _tourney_move_next_round_contestant($bracket, $match, $from);
      tourney_match_save($next_match);
      return $next_match;
    }
  }
}

/**
 * Figure out who the contestant is for the contestant spot passed in.
 */
function _tourney_move_next_round_contestant($bracket, $match, $from) {
  // Set the contestant on the next match. Round robin does not care about
  // winners or losers.
  if ($match->tournament->format == 'RoundRobin') {
    return ($from != 'same') ? $match->contestant2 : $match->contestant1;
  }
  else {
    $winners = tourney_get_winner_counts($match);

    // Get the winner uid.
    $winner = array_keys($winners, max($winners));
    $winner_contestant = $winner[0];

    // Get the loser uid.
    $loser = array_keys($winners, min($winners));
    $loser_contestant = $loser[0];

    return ($bracket == 'bracket-bottom' && $from != 'same') ? $loser_contestant : $winner_contestant;
  }
}
