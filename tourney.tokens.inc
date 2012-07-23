<?php
/**
 * @file
 * Token integration.
 */

/**
 * Implements hook_token_info().
 */

function tourney_token_info() {
  $info['tokens']['tourney_match'] = array(
    'bracket_name' => array(
      'name' => t('Bracket Name'),
      'description' => t('Name of bracket this match is in'),
    ),
    'round_name' => array(
      'name' => t('Round Name'),
      'description' => t('Name of round this match is in'),
    ),
    'round_number' => array(
      'name' => t('Round Number'),
      'description' => t('Round number this match is in')
    ),
    'match_number' => array(
      'name' => t('Match Number'),
      'description' => t('Number of this match in its tournament'),
    ),
    'contestant_1' => array(
      'name' => t('Contestant 1'),
      'description' => t('Contestant in slot 1 of the match'),
    ),
    'contestant_2' => array(
      'name' => t('Contestant 2'),
      'description' => t('Contestant in slot 2 of the match'),
    ),
    
  );
  $info['tokens']['tourney_game'] = array(
    'game_number' => array(
      'name' => t('Game Number'),
      'description' => t('Game number in match'),
    ),
  );
  return $info;
}

function tourney_tokens($type, $tokens, $data = array(), $options = array()) {
  $replacements = array();
  $sanitize = !empty($options['sanitize']);

  if ( $type == 'tourney_match' && !empty($data['tourney_match']) ) {
    $match = $data['tourney_match'];
    if ( !isset($match->id) ) return;
    $match->getMatchInfo();
    foreach ( $tokens as $name => $original ) {
      switch ( $name ) {
        case 'bracket_name':
          $replacements[$original] = $match->matchInfo['bracket']['title'];
          break;
        case 'round_name':
          $replacements[$original] = t('Round') . ' ' . $match->matchInfo['round'] . ' ';
          break;
        case 'round_number':
          $replacements[$original] = $match->matchInfo['round']['id'];
          break;
        case 'match_number':
          $replacements[$original] = $match->matchInfo['id'];
          break;
        case 'contestant_1':
          $contestant = $match->getContestant(1);
          $replacements[$original] = $contestant ? $contestant->entity_name : 'TBD';
          break;
        case 'contestant_2':
          $contestant = $match->getContestant(2);
          $replacements[$original] = $contestant ? $contestant->entity_name : 'TBD';
          break;
      }
    }
  } else if ( $type == 'tourney_game' && !empty($data['tourney_game']) ) {
    foreach ( $tokens as $name => $original ) {
      switch ( $name ) {
        case 'game_number':
          //the game id increments by 2, for display purposes we need to subtract 1 when the id > 1
          $id = array_pop(explode('-', $data['tourney_game']->name));
          $replacements[$original] = $id == 1 ? $id : $id - 1;
          break;
      }
    }
  }

  return $replacements;
}