<?php

/**
 * @file
 *
 * Renders the html for each round of a round robin tournament
 * 
 * @param $tournament
 *   The tournament object
 * @param $matches
 *   An array of all the rounds and matches in a tournament.
 */
function tourney_render_tourney_roundrobin($tournament, $matches) {
  // Render the standings table.
  $output = theme('tourney_roundrobin_standings', array(
    'tournament' => $tournament,
    'matches' => $matches
  ));
  
  foreach ($matches['bracket-roundrobin'] as $machine_name => $round) {
    $output .= "<h3 id='$machine_name'>" . str_replace('-', ' ', ucfirst($machine_name)) . "</h3>";
    foreach ($round as $match) {
      $output .= tourney_matchblock($match['match']);
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
  $contestants = tourney_get_contestants($vars['tournament']);
  $standings = tourney_get_standings($vars['tournament']);
  $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
  $table['rows'] = array();
  
  uasort($standings, 'tourney_roundrobin_standings_sort');

  foreach ( $standings as $id => $standing ) {
    $table['rows'][] = array($contestants[$id], $standing['total'], $standing['wins'], $standing['losses']);
  }

  return theme('table', $table);
}