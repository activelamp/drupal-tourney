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
  $winners = $match->fetchRanks(TRUE);

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

    // If the match already has a winner even though we've not met our quotient, that means a game was changed
    // We need to visit the next matches and clear the necessary contestants
    if ( $match->getWinner() ) {
      if ( $match->next_match_winner ) {
        $wmatch = $match->getTournament()->getMatchByName($match->next_match_winner);
        $wmatch->removeContestant(tourney_get_name_part($match->next_match_winner, 'contestant'));
      }
      if ( $match->next_match_loser ) {
        $lmatch = $match->getTournament()->getMatchByName($match->next_match_loser);
        $lmatch->removeContestant(tourney_get_name_part($match->next_match_loser, 'contestant'));
      }
    }

    // Check all our played games to make sure they've been won.
    // If one of them hasn't, that means we don't need to make a new game to continue the match
    $create_new_game = TRUE;
    foreach ( $match->getGameIds() as $game_id ) {
      $game = tourney_game_load($game_id);
      if ( !$game->getWinner() ) {
        $message = t('Game updated, but not all games have a winner selected. Action not completed.');
        watchdog('tourney', $message);
        $create_new_game = FALSE;
      }
    } 

    // If all our games are played, then we'll need to make a new game to continue the match
    if ($create_new_game) {
      $game_count = count($match->getGameIds());
      $game = new TourneyGame(array(
        'type' => 'game',
        'title' => 'game-' . ++$game_count,
      ));
      $game->save();
      // Add the relationship to the game.
      $rid = tourney_create_relation('has_game', 'tourney_match', $match->id, 'tourney_game', $game->id);
      $message = t('A new game has been created: !game', array('!game' => l($game->title, 'tourney/game/' . $game->id)));
    }
  }
  elseif ($match->getTournament()->format != 'RoundRobin') {
    // Clear the gameIds so they can be reloaded
    $match->gameIds = NULL;
    // Check all our games starting from the back to find the games that haven't been won
    // Since we've already won the match, we're going to remove all the games that no longer serve purpose for the match
    foreach ( array_reverse($match->getGameIds()) as $game_id ) {
      $game = tourney_game_load($game_id);
      // If we've run across a won game, it's almost certain the rest of the first matches have been played, so we don't need to continue
      if ( $game->getWinner() ) break;
      // @todo: find a way to just load in a single relation based on two related entities
      // This code just runs through all the related games to the match to find the specific relation for the game to delete, and removes it
      $relations = relation_query('tourney_match', $match->id)->entityCondition('bundle', 'has_game')->execute();
      foreach ( array_reverse($relations) as $relation ) {
        $relation = relation_load($relation->rid); 
        $relation_game = tourney_game_load($relation->endpoints['und'][1]['entity_id']);
        if ( $relation_game->id == $game_id ) {
          $relation_game->delete();
          relation_delete($relation->rid);
          break;
        }
      }
    }
    // With our fancy new oop methods, no more messy contestant movements!
    $match->moveContestants();
    $match->determineWinner();
    $message = t('Players moved to next round');
  }
  else {
    $match->determineWinner();
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
  $winners = $match->fetchRanks();
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
  $match->save();
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
      dpm($next_match);
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
  if ($match->getTournament()->format == 'RoundRobin') {
    return ($from != 'same') ? $match->getContestant(2) : $match->getContestant(1);
  }
  else {
    $winners = $match->fetchRanks();

    // Get the winner uid.
    $winner = array_keys($winners, max($winners));
    $winner_contestant = $winner[0];

    // Get the loser uid.
    $loser = array_keys($winners, min($winners));
    $loser_contestant = $loser[0];

    return ($bracket == 'bracket-bottom' && $from != 'same') ? $loser_contestant : $winner_contestant;
  }
}
