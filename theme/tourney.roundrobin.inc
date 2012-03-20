<?php

function tourney_render_tourney_roundrobin($tournament, $matches) {
  $rounds = array();
  while ( $matches ) {
    $this_round = array();
    for ( $i = 0; $i < $tournament->players / 2; $i++ ) $this_round[] = array_shift($matches);
    $rounds[] = $this_round;
  }

  $output = '';
  foreach ( $rounds as $k => $round ) {
    $output .= "<h3 id='round-$k'>Round " . ($k+1) . "</h3>";
    foreach ( $round as $match ) $output .= tourney_matchblock($match['match']);
    $output .= '<div class="clearfix"></div>';
  }
  return $output;
}