<?php

/**
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
    $output .= "<h3 id='$machine_name'>". str_replace('-', ' ', ucfirst($machine_name)) ."</h3>";
    foreach ($round as $match) {
      $output .= tourney_matchblock($match['match']);
    } 
    $output .= '<div class="clearfix"></div>';
  }
  return $output;
}

/**
 * Renders the html for the leader board for a round robin tourney
 */
function theme_tourney_roundrobin_standings($vars) {
  $contestants = tourney_get_contestants($vars['tournament']);
  $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
  $table['rows'] = array();
  dpm($vars);
  dpm($contestants);
  foreach ($contestants as $contestant) {
    $table['rows'][] = array($contestant, 0, 0, 0);
  }

  return theme('table', $table);
}