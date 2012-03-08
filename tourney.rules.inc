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
  );
  
  return $actions;
}

/**
 * Action function to get the number of won games.
 */
function tourney_action_get_game_count($match) {
  // Get the number of games each contestant has won.
  $winners = _tourney_action_get_winner_counts($match);

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
  // Create a new game if enough games have not been won yet.
  if ($quotient >= 2) {
    // Default action to create a new game.
    $create_new_game = true;
    
    // Ensure every game has a winner selected
    foreach ($match->games_played as $game_id) {
      $game = tourney_game_load($game_id);
      if (!$game->winner) {
        $message = t('Game updated, but not all games have a winner selected. Action not completed.');
        watchdog('tourney', $message);
        $create_new_game = false;
      }
    }
    if ($create_new_game) {
      $game_count = count($match->games_played);
      $game = new stdClass();
      $game->type = 'game';
      $game->title = 'game-'. ++$game_count;
      tourney_game_save($game);
      // Add the relationship to the game.
      $rid = tourney_create_relation('has_game', 'tourney_match', $match->id, 'tourney_game', $game->id);

      $message = t('A new game has been created: !game', array('!game' => l($game->title, 'tourney/game/'. $game->id)));
    }
  }
  else {
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
  
  return array(
    'action_message' => $message,
  );
}

function _tourney_set_match_winner($match) {
  $winners = _tourney_action_get_winner_counts($match);
  $winner = array_keys($winners, max($winners));
  list($type, $id) = explode(".", $winner[0]);
  $wobj = entity_load($type, array($id));
  $winner = array_pop($wobj); 
  
  // @todo what do we do for a winner thing here... type.id or name or what? using name for now
  $match->winner = $winner->_entity_name;
  tourney_match_save($match);
}

function _tourney_move_next_round($match, $bracket, $round, $match_name, $contestant, $from = 'same') {
  // Get the tournament object this match is in.
  $tournament = relation_get_related_entity('tourney_match', $match->id, 'has_match');
  
  // Get all the relations in the destination round.
  $query = relation_query('tourney_tournament', $tournament->id);
  $query->entityCondition('bundle', 'has_match')
        ->fieldCondition('round', 'value', $round)
        ->fieldCondition('bracket', 'value', $bracket);
  $results = $query->execute();

  // Find the match the contestant should go to
  foreach ($results as $relation) {
    $r = relation_load($relation->rid);
    $next_match = $r->endpoints[LANGUAGE_NONE][1]['entity_type'] == 'tourney_match'
      ? tourney_match_load($r->endpoints[LANGUAGE_NONE][1]['entity_id'])
      : NULL;

    if ($next_match->title == $match_name) {
      $winners = _tourney_action_get_winner_counts($match);
      
      // Get the winner uid.
      $winner = array_keys($winners, max($winners));
      $winner_contestant = $winner[0];

      // Get the loser uid.
      $loser = array_keys($winners, min($winners));
      $loser_contestant = $loser[0];
      
      $contestant = str_replace('-', '', $contestant);
      $next_match->$contestant = ($bracket == 'bracket-bottom' && $from != 'same') ? $loser_contestant : $winner_contestant;
      
      tourney_match_save($next_match);
      break;
    }
  }
}

/**
 * Figure out how many games each contestant has won.
 * 
 * @param $match
 *   The match object to check games on
 * @return
 *   Returns an array keyed by uid with value being the number of won games.
 */
function _tourney_action_get_winner_counts($match) {
  if (!$cache = cache_get('tourney_winner_count_'. $match->id)) {
    $games = array();
    
    // Set up the default count array.
    $winners = array(
      $match->contestant1->_entity_type .'.'. $match->contestant1->_entity_id => 0,
      $match->contestant2->_entity_type .'.'. $match->contestant2->_entity_id => 0,
    );
    
    foreach ($match->games_played as $entity_id) {
      $games[$entity_id] = tourney_game_load($entity_id); 
      // Check to make sure the game has a winner
      if ($games[$entity_id]->winner) {
        // Tally up the winners in array keyed by uid
        $winners[$games[$entity_id]->winner] = 
          array_key_exists($games[$entity_id]->winner, $winners) 
          ? ++$winners[$games[$entity_id]->winner] : 1; 
      }
    }
    cache_set('tourney_winner_count_'. $match->id, $winners);
    return $winners;
  }
  return !empty($cache->data) ? $cache->data : array();
}
