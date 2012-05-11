<?php

/**
 * @file
 */
 
/**
 * Renders the html for each round of a round robin tournament
 * 
 * @param $tournament
 *   The tournament object
 * @param $matches
 *   An array of all the rounds and matches in a tournament.
 */
function theme_tourney_render_tourney_roundrobin($vars) {
  extract($vars);
  drupal_add_js(drupal_get_path('module', 'tourney') . '/theme/js/single.js');
  // Render the standings table.
  $output = theme('tourney_roundrobin_standings', array(
    'tournament' => $tournament,
    'matches' => $matches
  ));
  
  foreach ($matches['roundrobin'] as $r => $round) {
    $output .= "<h3 id='round-$r'>Round $r</h3>";
    foreach ($round as $match) {
      $output .= theme('tourney_matchblock', array('match' => $match));
    } 
    $output .= '<div class="clearfix"></div>';
  }
  return $output;
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

/**
 * Renders the html for the leader board for a round robin tourney
 */
function theme_tourney_roundrobin_standings($vars) {
  $contestants = $vars['tournament']->getContestants();
  $standings = $vars['tournament']->getStandings();
  $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
  $table['rows'] = array();
  
  uasort($standings, 'tourney_roundrobin_standings_sort');

  foreach ( $standings as $id => $standing ) {
    $table['rows'][] = array($contestants[$id]->entity_name, $standing['total'], $standing['wins'], $standing['losses']);
  }

  return theme('table', $table);
}