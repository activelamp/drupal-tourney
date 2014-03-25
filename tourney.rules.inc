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