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