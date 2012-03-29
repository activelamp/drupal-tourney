<?php

function tourney_render_tourney_roundrobin($tournament, $rounds) {
  $output = '';
  foreach ( $rounds as $k => $round ) {
    $output .= "<h3 id='round-$k'>Round " . ($k+1) . "</h3>";
    foreach ( $round as $match ) $output .= tourney_matchblock($match['match']);
    $output .= '<div class="clearfix"></div>';
  }
  return $output;
}