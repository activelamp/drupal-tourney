<?php

function tourney_render_tourney_single($tournament, $matches) {
  $rounds = array_values($matches['bracket-top']);
  $output = tourney_single_tree($rounds, $tournament->players > 8);
  return $output;
}

function tourney_single_tree($rounds, $small = FALSE) {
  $output = '';
  foreach ( $rounds as $r => $round ) {
    $last = $r >= count($rounds) -1;
    $top_pad = pow(2, $r)-1;
    $mid_pad = $small ? $top_pad * 2 : pow(2, $r+1)-1;

    $output .= '<div class="round round-' . $r . '">';
    $output .= tourney_spacer($top_pad, null, $small);
    foreach ( array_values($round) as $m => $match ) {
      $output .= tourney_matchblock($match['match'], $last ? '' : ( $m % 2 ? ( $mid_pad == 0 ? 'topleft evenright,special' : 'topleft' ) : 'bottomleft' ), $small);
      if ( $m == count($round) - 1 ) 
        $output .= tourney_spacer($top_pad, null,  $small);
      else
        $output .= tourney_spacer($mid_pad, !($m%2), $small);
    }
    $output .= '</div>';
  } 
  return $output;
}