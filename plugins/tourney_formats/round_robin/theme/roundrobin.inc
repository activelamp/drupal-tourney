<?php

/**
 * @file
 */

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