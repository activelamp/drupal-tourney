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

function tourney_render_theme($existing, $type, $theme, $path) {
  return array(
    'tourney_matchblock' => array(
      'variables' => array('match' => NULL, 'spacing' => 0),
      'file' => 'tourney.inc',
      'path' => $path .'/theme'
    ),
    'tourney_spacer' => array(
      'file' => 'tourney.inc',
      'path' => $path .'/theme',
    ),
  );
}

function theme_tourney_matchblock($vars) {
  $match = $vars['match']['match'];
  $spacing = $vars['spacing'];
  $contestant1 = is_object($match->contestant1) ? $match->contestant1->_entity_name : 'team 1';
  $contestant2 = is_object($match->contestant2) ? $match->contestant2->_entity_name : 'team 2';
  $title = $match->title .' '. $match->id;

  $output  = '';
  $output .= '<tr class="contestant"><td>' . $contestant1 . '</td></tr>';
  if ( $spacing ) 
    $output .= '<tr class="spacing"><td>' . theme('tourney_spacer', $spacing) . '</td></tr>';
  $output .= '<tr class="contestant"><td>' . $contestant2 . '</td></tr>';

  return tourney_table_wrap($output, 'match');
}

function theme_tourney_spacer($height) {
  $output  = '';
  for ( $i = 0; $i < $height; $i++ ) $output .= '<div class="spacer"></div>';
  return tourney_table_wrap($output, 'spacer');
}

function tourney_table_wrap($content, $class = null) {
  if ( is_array($content) || substr($content, 0, 3) !== '<tr' )
    $content = '<tr><td>' . implode('</td></tr><tr><td>', is_array($content) ? $content : array($content)) . '</td></tr>';
  return '<table border="0" cellpadding="0" cellspacing="0" class="'.$class.'">'.$content.'</table>';
}