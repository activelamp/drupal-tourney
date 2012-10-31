<?php

/**
 * @file
 * Preprocess Regular Season templates
 */

/**
 * Round Robin preprocessor.
 */
function template_preprocess_tourney_regularseason(&$vars) {
  $plugin = &$vars['plugin'];
  $vars['rounds'] = $plugin->structure;

  $plugin_dir = drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/regular_season';
  drupal_add_css($plugin_dir . '/theme/regularseason.css');
}

/**
 * Regular Season standings table preprocessor.
 */
function template_preprocess_tourney_regularseason_standings(&$vars) {
  $plugin = $vars['plugin'];
  $table = array();
  $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
  $table['rows'] = array();

  $slots = $vars['plugin']->numContestants;

  // If we have a tournament object, load all contestants in tournament.
  if (is_object($plugin->tournament)) {
    $contestants = $plugin->tournament->getContestants();
    $standings = $plugin->tournament->getStandings();
    uasort($standings, 'tourney_regularseason_standings_sort');
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
  //$vars['standings'] = 'These are the standings';
}
