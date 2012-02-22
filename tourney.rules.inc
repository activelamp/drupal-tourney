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
    $new_game = true;
    
    // Ensure every game has a winner selected
    foreach ($match->games_played as $game_id) {
      $game = tourney_game_load($game_id);
      if (!$game->winner) {
        watchdog('tourney', t('Game updated, but not all games have a winner selected.'));
        $message = t('Game updated, but not all games have a winner selected.');
        $new_game = false;
      }
    }
    if (!$new_game) {
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
    $message = t('Players moved to next round');
  }
  
  return array(
    'action_message' => $message,
  );
}

/**
 * Figure out how many games each contestant has won.
 */
function _tourney_action_get_winner_counts($match) {
  $games = array();
  $winners = array();
  
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
  return $winners;
}