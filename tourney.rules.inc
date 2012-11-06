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
    'tourney_action_set_regularseason_matches' => array(
      'label' => t('Set contestant in all matches for Regular Season.'),
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
 * Implements hook_rules_condition_info().
 */
function tourney_rules_condition_info() {
  return array(
    'tourney_condition_match_has_contestants' => array(
      'label' => t('All match contestants are set.'),
      'group' => t('Tourney Rules'),
      'parameter' => array(
        'match' => array(
          'type' => 'tourney_match',
          'label' => t('The match to check for contestants on.'),
        ),
      ),
    ),
  );
}

/**
 * Rules condition: Check to see if match has both contestants set.
 * 
 * @param $match
 *   The match object being saved, passed in from Rules.
 */
function tourney_condition_match_has_contestants($match) {
  $return = FALSE;
  $contestants = $match->getMatchContestants();
  $count = count($contestants);
  
  if (($count == 2) || ($match->matchInfo['bye'] == TRUE && $count > 0)) {
    $return = TRUE;
  }
  return $return;
}

/**
 * Action function to get the number of won games.
 */
function tourney_action_get_game_count($match) {
  // Get the number of games each contestant has won.
  $winners = $match->fetchRanks(TRUE);

  return !empty($winners) ? array(
    // This should never return 0, can't divide by zero.
    'number_of_won_games' => max($winners) > 0 ? max($winners) : 1,
  ) : 1;
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
  // This static variable gets set when a match is being reset. If the variable
  // is set, then we need to skip this rules logic.
  $resetMatch = &drupal_static('resetMatch', FALSE);
  if ($resetMatch) {
    return;
  }
  
  $plugin = $match->getTournament()->tourneyFormatPlugin;
  $message = '';
  // Create a new game if enough games have not been won yet.
  if ($quotient >= 2) {
    // If the match already has a winner even though we've not met our quotient,
    // that means a game was changed. We need to get the next matches and clear
    // the necessary contestants
    if ($match->getWinner()) {
      if ($match->nextMatch()) {
        $match->getTournament()->winner = '';
        $match->getTournament()->save();
        $match->nextMatch()->status = 1;
        $match->nextMatch()->removeContestant($match->getWinnerEntity());
        $match->nextMatch()->save();
      }
      if ($match->nextMatch('loser')) {
        $match->nextMatch('loser')->removeContestant($match->getLoserEntity());
      }
    }

    // If all our games are played and won, then we'll need to make a new game to continue the match
    if ($match->checkGames() == TRUE) {
      $game = $match->addGame();    
      $message = t("A new game has been created for {$game->title}.");
    }
    else {
      $message = t('Game updated, but not all games have a winner selected. Action not completed.');
      watchdog('tourney', $message);
    }
  }
  else {
    if ($plugin->moveWinners == TRUE) {
      $match->getTournament()->matchIsWon($match);
      $message = t('Players moved to next round');    
    }
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
  $match->getTournament()->populateRoundRobinMatches($match->matchInfo['id']);
  $message = t('Players added to all rounds');
  
  return array(
    'action_message' => $message,
  );
}

/**
 * Recursive function that sets players in a RegularSeason tournament in all 
 * their matches once the first round has been setup.
 *
 * @param $match (object)
 *   The match the game belongs to
 */
function tourney_action_set_regularseason_matches($match) {
  $match->getTournament()->populateRegularSeasonMatches($match->matchInfo['id']);
  $message = t('Players added to all rounds');
  
  return array(
    'action_message' => $message,
  );
}