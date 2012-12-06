<?php

/*
 * @file
 * Preprocess Manual Upload templates
 */

/**
 * Manual Upload preprocessor.
 */
function template_preprocess_tourney_manualupload(&$vars) {
  $plugin = &$vars['plugin'];
  $vars['rounds'] = $plugin->structure;

  $plugin_dir = drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/manual_upload';
  drupal_add_css($plugin_dir . '/theme/manualupload.css');
}

/**
 * Manual Upload standings table preprocessor.
 */
function template_preprocess_tourney_manualupload_standings(&$vars) {
  $plugin = $vars['plugin'];
  $table = array();
  $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
  $table['rows'] = array();

  $slots = $vars['plugin']->numContestants;

  // If we have a tournament object, load all contestants in tournament.
  if (is_object($plugin->tournament)) {
    $contestants = $plugin->tournament->getContestants();
    $standings = $plugin->tournament->getStandings();
    uasort($standings, 'tourney_manualupload_standings_sort');
  } else {
    // Create fake contestants.
    for ($contestant = 1; $contestant <= $slots; $contestant++) {
      $contestants[$contestant] = (object) array('name' => 'Contestant ' . $contestant);
    }
  }

  foreach ($standings as $id => $standing) {
    $contestant = $contestants[$id];
    $games_played = $standings[$id]['total'] ? $standings[$id]['total'] : 0;
    $wins = $standings[$id]['wins'] ? $standings[$id]['wins'] : 0;
    $losses = $standings[$id]['losses'] ? $standings[$id]['losses'] : 0;
    $table['rows'][] = array($contestant->entity_name, $games_played, $wins, $losses);
  }

  $vars['standings'] = theme('table', $table);
}

/**
 * Manual Upload standings custom sorter by wins > losses > played first.
 *
 * @todo: Need to make this overribale in a better way.
 */
function tourney_manualupload_standings_sort($a, $b) {
  if ($a['wins'] != $b['wins']) {
    return $a['wins'] < $b['wins'] ? 1 : -1;
  }
  else if ($a['losses'] != $b['losses']) {
    return $a['losses'] < $b['losses'] ? 1 : -1;
  }
  else {
    return $a['first'] < $b['first'] ? 1 : -1;
  }
}
