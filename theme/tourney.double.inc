<?php

function tourney_render_tourney_double($tournament, $matches) {
  $output = '';

  if ( array_key_exists('bracket-top', $matches) )
    $output  = '<div class="bracket bracket-top">' 
            . tourney_dummy_rounds(count($matches['bracket-bottom']) - count($matches['bracket-top']), $tournament->players)
            . tourney_double_top_bracket(array_values($matches['bracket-top']), $tournament->players) . '</div>';
  if ( array_key_exists('bracket-champion', $matches) )
    $output .= '<div class="bracket bracket-champion">' . tourney_double_champion_bracket($tournament, array_values($matches['bracket-champion']), 1) . '</div>';
  if ( array_key_exists('bracket-bottom', $matches) )
    $output .= '<div class="bracket bracket-bottom">' . tourney_double_bottom_bracket(array_values($matches['bracket-bottom'])) . '</div>';
  return $output;
}

function tourney_dummy_rounds($count, $height, $small = 1) {
  $round = '<div class="round round-dummy">' . tourney_spacer($height, 0, $small) . '</div>';
  return str_repeat($round, $count);
}


function tourney_double_top_bracket($rounds, $players) {
  $output = '';
  foreach ( $rounds as $r => $round ) {
    $last = $r >= count($rounds) -1;
    $top_pad = pow(2, $r)-1;
    $mid_pad = $top_pad * 2;

    $output .= '<div class="round round-' . $r . '">';
    $output .= tourney_spacer($top_pad, null, 1);
    foreach ( array_values($round) as $m => $match ) {
      $output .= tourney_matchblock($match['match'], $last ? 'bottomleft' : ( $m % 2 ? ( $mid_pad == 0 ? 'topleft evenright,special' : 'topleft' ) : 'bottomleft' ), 1);
      if ( $m == count($round) - 1 ) 
        if ( $r == count($rounds) -1 ) {
          $pads = $players/4-1;
          $output .= tourney_spacer($pads, 'vertical', 1);
          $output .= tourney_spacer(1, 'vertical evenright,special', 1);
          $output .= tourney_spacer($pads, 'vertical', 1);
        }
        else 
          $output .= tourney_spacer($top_pad, null,  1);  
      else
        $output .= tourney_spacer($mid_pad, !($m%2), 1);
    }
    $output .= '</div>';
  } 
  return $output;
}

function tourney_double_bottom_bracket($rounds) {
  $small = 1;
  $output = '';
  foreach ( $rounds as $r => $round ) {
    $last = $r >= count($rounds) -1;
    $cr = floor($r/2);
    $top_pad = $r == 0 ? 1 : ( $r % 2 ? 0 : pow(2, $cr-1) );
    $mid_pad = pow(2, $cr+1)-2;

    $output .= '<div class="round round-' . $r . '">';
    $output .= tourney_spacer($top_pad, $top_pad > 1, $small, 0);
    foreach ( array_values($round) as $m => $match ) {
      $output .= tourney_matchblock($match['match'], 
        $r % 2 ? ( 
          $m % 2 ? ( 
            $mid_pad ? 'topleft' : 'topleft special,evenright' ) 
          : ( count($round) > 1 ? 'bottomleft' : 'topleft' ) ) 
        : ( $top_pad > 1 ? 'topleft' : 'topleft special,evenright' ), 1);
      if ( $m == count($round) - 1 ) 
        $output .= tourney_spacer($top_pad, null,  $small);
      else
        $output .= tourney_spacer($mid_pad, !($m%2) && $r%2, $small);
    }
    $output .= '</div>';
  } 
  return $output;
}

function tourney_double_champion_bracket($tournament, $rounds) {
  $champ = array_pop($rounds[0]);
  $champ = $champ['match'];

  $output  = '';
  $output .= '<div class="round round-champion">';
  $output .= tourney_spacer(3 * ($tournament->players / 4)-1, null, 1);
  $output .= tourney_matchblock($champ, 'horizontal', 1);
  $output .= '</div>';
  $output .= '<div class="round round-extra">';
  $output .= tourney_spacer(3 * ($tournament->players / 4)-1, null, 1);
  $output .= tourney_matchblock($champ, null, 1);
  $output .= '</div>';
  return $output;
}