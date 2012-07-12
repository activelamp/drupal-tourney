<?php

/*
 * @file
 * Preprocess Round Robin templates
 */

/**
 * 
 */
function template_preprocess_tourney_roundrobin(&$vars) {
  $plugin = &$vars['plugin'];
  $vars['rounds'] = $plugin->structure;
  
  $plugin_dir = drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/special_round';
  drupal_add_css($plugin_dir . '/theme/roundrobin.css');
}

/**
 * 
 */
function template_preprocess_tourney_roundrobin_standings(&$vars) {
  $plugin = $vars['plugin'];
  $table = array();
  $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
  $table['rows'] = array();
  
  $slots = $vars['plugin']->numContestants;
  
  // If we have a tournament object, load all contestants in tournament.
  if (is_object($plugin->tournament)) {
    $contestants = $plugin->tournament->getContestants();
  }
  else {
    // Create fake contestants.
    for ($contestant = 1; $contestant <= $slots; $contestant++) {
      $contestants[$contestant] = (object) array('name' => 'Contestant ' . $contestant);
    }
  }
  
  $games_played = $wins = $losses = 0;
  foreach ($contestants as $contestant) {
    $table['rows'][] = array($contestant->name, $games_played, $wins, $losses);
  }
  
  $vars['standings'] = theme('table', $table);
}


function tourney_roundrobin_standings_sort($a, $b) {
  if ( $a['wins'] != $b['wins'] ) {
    return $a['wins'] < $b['wins'] ? 1 : -1;
  }
  elseif ( $a['losses'] != $b['losses'] ) {
    return $a['losses'] < $b['losses'] ? 1 : -1;
  }
  else {
    return $a['first'] < $b['first'] ? 1 : -1;
  }
}

// function theme_tourney_roundrobin($vars) {
  // $tournament = $vars['tournament'];

  // Render the standings table.
  // $output = theme('tourney_roundrobin_standings', array(
  //   'tournament' => $tournament
  // ));

  // $matches = $tournament->buildMatches();
  // foreach ($tournament->data['bracket-roundrobin']['rounds'] as $round_name => $round) {
  //   $r = substr(strstr($round_name, '-'), 1);
  //   $output .= "<h3 id='round-$r'>Round $r</h3>";
  //   foreach ($round['matches'] as $match_name => $match_callbacks) {
  //     $match = $tournament->tourneyFormatPlugin
  //       ->$match_callbacks['current_match']['callback']($match_callbacks['current_match']['args']);
  // 
  //     $output .= theme('tourney_matchblock', array('match' => $match));
  //   }
  //   $output .= '<div class="clearfix"></div>';
  // }
  // return $output;
// }

/**
 * Renders the html for the leader board for a round robin tourney
 */
// function theme_tourney_roundrobin_standings($vars) {
//   $table = array();
//   $tournament = $vars['tournament'];
// 
//   // $contestants = $tournament->getContestants();
//   // $standings = $tournament->getStandings();
// 
//   $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
//   $table['rows'] = array();
// 
//   // uasort($standings, 'tourney_roundrobin_standings_sort');
// 
//   foreach ($contestants as $id => $contestant) {
//     $table['rows'][] = array($contestant->entity_name, $standings[$id]['total'] ?: 0, $standings[$id]['wins'] ?: 0, $standings[$id]['losses'] ?: 0);
//   }
// 
//   if (!$table['rows']) {
//     return NULL;
//   }
//   return theme('table', $table);
// }