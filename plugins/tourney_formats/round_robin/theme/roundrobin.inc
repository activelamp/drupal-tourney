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
  
  $plugin_dir = drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/round_robin';
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
    $standings = $plugin->tournament->getStandings();
    uasort($standings, 'tourney_roundrobin_standings_sort');
  }
  else {
    // Create fake contestants.
    for ($contestant = 1; $contestant <= $slots; $contestant++) {
      $contestants[$contestant] = (object) array('name' => 'Contestant ' . $contestant);
    }
  }
  
  
  foreach ($contestants as $id => $contestant) {
    if (array_key_exists($id, $standings)) {
      $games_played = $standings[$id]['total'] ? $standings[$id]['total'] : 0;
      $wins = $standings[$id]['wins'] ? $standings[$id]['wins'] : 0;
      $losses = $standings[$id]['losses'] ? $standings[$id]['losses'] : 0;
    }
    else {
      $games_played = $wins = $losses = 0;
    }
    
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