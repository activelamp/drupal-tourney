<?php

function tourney_render_tourney_single($tournament, $matches) {
  $rounds = array_values($matches['bracket-top']);
  $output = '';
  foreach ( $rounds as $k => $round ) {
    $this_round = array();
    $spacing = theme('tourney_spacer', $pad = (pow(2, $k)-1));
    if ( $spacing ) $this_round[] = $spacing;
    foreach ( $round as $match ) 
      $this_round[] = theme('tourney_matchblock', array('match' => $match, 'spacing' => $pad-1));
    $output .= tourney_table_wrap($this_round, 'round');
  } 
  return $output;
}