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
  $output = '';
  foreach ($matches['bracket-roundrobin'] as $machine_name => $round) {
    $output .= "<h3 id='$machine_name'>". str_replace('-', ' ', ucfirst($machine_name)) ."</h3>";
    foreach ($round as $match) {
      $output .= tourney_matchblock($match['match']);
    } 
    $output .= '<div class="clearfix"></div>';
  }
  return $output;
}