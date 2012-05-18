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
  dpm($match);
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
  dpm($match);
  $message = '';
  
  // Create a new game if enough games have not been won yet.
  if ($quotient >= 2) {

    // If the match already has a winner even though we've not met our quotient,
    // that means a game was changed. We need to get the next matches and clear
    // the necessary contestants
    if ($match->getWinner()) {
      if ($match->nextMatch()) {
        $match->nextMatch()->removeContestant($match->getWinnerEntity());
      }
      if ($match->nextMatch('loser')) {
        $match->nextMatch('loser')->removeContestant($match->getLoserEntity());
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
        'type'  => 'game',
        'title' => 'Game ' . ++$game_count,
        'name'  => $match->name . '_game-' . ++$game_count,
      ));
      $game->save();
      // Add the relationship to the game.
      $rid = tourney_create_relation('has_game', 'tourney_match', $match->id, 'tourney_game', $game->id);
      $message = t('A new game has been created: !game', array('!game' => l($game->title, 'tourney/game/' . $game->id)));
    }
  }
  elseif ($match->getTournament()->format != 'RoundRobinController') {
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
    tourney_action_set_all_matches($match);
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
  foreach ($match->getContestants() as $contestant) {
    $next = $match->nextMatch($contestant->slot);
    
    if ( $next === NULL ) continue;
    $slot = $match->getTournament()->tourneyFormatPlugin->getNextSlot($match, $contestant->slot);

    // If the slot is already taken, don't do anything
    if ( $next->getContestant($slot) ) continue;
    $next->addContestant($contestant, $slot);
    // Recurse
    tourney_action_set_all_matches($next);
  }

  $message = 'Players added to all rounds';
  
  return array(
    'action_message' => $message,
  );
}
