<?php

function theme_tourney_render_tourney($vars) {
  $types = array(
    'SingleElimination' => 'single',
    'DoubleElimination' => 'double',
    'RoundRobin'        => 'roundrobin',
  );
  $tournament = $vars['tournament'];
  $format     = $types[$tournament->format];
  $matches    = _tourney_tournament_build_matches($tournament);
  $output     = "<div class='tourney tourney-$format'>";
  module_load_include('inc', 'tourney', "theme/tourney.$format");
  if ( function_exists($f = "tourney_render_tourney_$format") ) $output .= call_user_func_array($f, array($tournament, $matches));
  $output    .= "</div>";
  return $output;
}

function tourney_flow_control($flows) {
  if ( !$flows ) return;
  $output = '<div class="flow"><div class="flow-control '
        .  implode('"></div><div class="flow-control ', explode(' ', $flows))
        .  '"></div></div>';  
  return str_replace(',', ' ', $output);
}

function tourney_matchblock($match, $flow = FALSE) {
  $editable = !(is_object($match->contestant1) && is_object($match->contestant2)) && 
    ($match->previous_match1 == 'manual' || $match->previous_match1 == 'manual');
  $contestant1 = is_object($match->contestant1) ? $match->contestant1->_entity_name : 'team 1';
  $contestant2 = is_object($match->contestant2) ? $match->contestant2->_entity_name : 'team 2';
  $title       = $match->title . ' ' . $match->id;

  $output  = '<div class="match">';
  $output .= tourney_flow_control($flow);
  $output .= '<div class="contestant contestant-1">' . $contestant1 . ( $match->winner == $contestant1 ? ' ♔' : '' ) . '</div>';
  $output .= '<div class="contestant contestant-2">' . $contestant2 . ( $match->winner == $contestant2 ? ' ♔' : '' ) . '</div>';
  $output .= '</div>';
  return l($output, "tourney/match/{$match->id}" . ( $editable ? "/edit" : ''), array(
    'html' => TRUE, 
    'attributes' => array('class' => 'match-link'), 
    'query' => $editable ? array('destination' => "tourney/match/{$match->id}") : array()));
}

function tourney_spacer($times = 1, $flow = FALSE, $small = FALSE, $y = 1) {
  $div = '<div class="match match-' . ( $small ? 'half-' : '' ) . 'spacer">%s</div>';

  if ( $times <= 0 ) return;
  if ( $flow ) {
    $half    = floor($times/2);
    $normal  = sprintf($div, tourney_flow_control('vertical'));
    $output  = $y == 0 ? sprintf($div, '') . str_repeat($normal, $half - 1 ) : str_repeat($normal, $half);
    $output .= sprintf($div, is_string($flow) ? tourney_flow_control($flow) : ( $times % 2 ? tourney_flow_control('topright bottomright') : tourney_flow_control('evenright') ));
    if ( !($times % 2) ) $half--;
    $output .= str_repeat($normal, $half);
  }
  else
    $output  = str_repeat(sprintf($div, ''), $times);
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
      $output .= tourney_matchblock($match['match'], $last ? '' : ( $m % 2 ? ( $mid_pad == 0 ? 'topleft evenright,special' : 'topleft' ) : 'bottomleft' ) );
      if ( $m == count($round) - 1 ) 
        $output .= tourney_spacer($top_pad, null,  $small);
      else
        $output .= tourney_spacer($mid_pad, !($m%2), $small);
    }
    $output .= '</div>';
  } 
  return $output;
}