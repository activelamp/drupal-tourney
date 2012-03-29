<?php

/**
 * Theme function to return HTML output for a given tournament, pulls includes
 * based on tournament type
 * 
 * @param $vars
 *   Should be a single-key array containing the tournament
 * @return
 *   Tree in HTML output
 */
function theme_tourney_render_tourney($vars) {
  module_load_include('inc', 'tourney', 'includes/tourney.tournament');
  $types = array(
    'SingleElimination' => 'single',
    'DoubleElimination' => 'double',
    'RoundRobin'        => 'roundrobin',
  );
  $tournament = $vars['tournament'];
  $format     = $types[$tournament->format];
  if ( array_key_exists('matches', $vars) )
    $matches  = $vars['matches'];
  else
    $matches  =  _tourney_tournament_build_matches($tournament);
  $output     = "<div class='tourney tourney-$format'>";
  module_load_include('inc', 'tourney', "theme/type/$format");
  if ( function_exists($f = "tourney_render_tourney_$format") ) $output .= call_user_func_array($f, array($tournament, $matches));
  $output    .= "</div>";
  return $output;
}

/**
 * Makes making specific flow boxes easy by wrapping a list of them with HTML
 * 
 * @param $flows
 *   List of flow types to wrap
 * @return
 *   Wrapped flow HTML 
 */
function tourney_flow_control($flows, $options = array()) {
  if ( !$flows ) return;
  $attributes = (!empty($options['attributes']) ? $options['attributes'] : array());
  $attributes['class'][] = 'flow';

  $output = '<div ' . drupal_attributes($attributes) . '><div class="flow-control '
        .  implode('"></div><div class="flow-control ', explode(' ', $flows))
        .  '"></div></div>';  
  return str_replace(',', ' ', $output);
}

/**
 * Renders a single match between two contestants
 * 
 * @param $match
 *   Match object
 * @param $flow
 *   Flow paths to use (the lines that connect and show relations between 
 *   matches in a tree)
 * @param $small
 *   Boolean to determine half-height matches
 * @return
 *   HTML link to either the edit or view match page
 */
function tourney_matchblock($match, $flow = FALSE, $small = 0) {
  $editable = !(is_object($match->contestant1) && is_object($match->contestant2)) && 
    ($match->previous_match1 == 'manual' || $match->previous_match1 == 'manual');
  $contestant1 = is_object($match->contestant1) ? $match->contestant1->_entity_name : 'team 1';
  $contestant2 = is_object($match->contestant2) ? $match->contestant2->_entity_name : 'team 2';
  $title       = $match->title . ' ' . $match->id;

  $output  = '<div class="match ' . ( $small ? 'small' : 'normal') . '">';
  if ( !$small )
    $output .= '<div class="match-title">' . $title . '</div>';

  // Add extra classes to the contestant and flow containers to aid in highlighting
  $contestant1_class = (!empty($match->contestant1) ? ' entity-' . $match->contestant1->_entity_type . '-' . $match->contestant1->_entity_id : '');
  $contestant2_class = (!empty($match->contestant2) ? ' entity-' . $match->contestant2->_entity_type . '-' . $match->contestant2->_entity_id : '');
  $winner_class = '';
  if (!empty($match->winner)) {
    if ($match->winner == $match->contestant1->name) $winner_class = $contestant1_class;
    if ($match->winner == $match->contestant2->name) $winner_class = $contestant2_class;
  }

  $output .= tourney_flow_control($flow, array('attributes' => array('class' => array($winner_class))));
  $output .= '<div class="contestant contestant-1 ' . $contestant1_class . '">' . $contestant1 . ( $match->winner == $contestant1 ? ' ♔' : '' ) . '</div>';
  $output .= '<div class="contestant contestant-2 ' . $contestant2_class . '">' . $contestant2 . ( $match->winner == $contestant2 ? ' ♔' : '' ) . '</div>';
  $output .= '</div>';
  return l($output, "tourney/match/{$match->id}" . ( $editable ? "/edit" : ''), array(
    'html' => TRUE, 
    'attributes' => array('class' => 'match-link'), 
    'query' => $editable ? array('destination' => "tourney/match/{$match->id}") : array()));
}

/**
 * Helper function to render spacing divs
 * 
 * @param $times
 *   Number of spacers to generate
 * @param $flow
 *   Either TRUE or String. If string, all flows are of the $flow type. If 
 *   boolean, generate default flow
 * @param $small
 *   Boolean to determine half width spacers
 * @param $y
 *   Special argument that should be used as a 'Y-position', but is currently 
 *   used to determine when a spacer is at the 0 Y-position for double 
 *   elimination render purposes only
 * @return
 *   Rendered HTML spacer output 
 */
function tourney_spacer($times = 1, $flow = FALSE, $small = FALSE, $y = 1) {
  $div = '<div class="match match-spacer '. ( $small ? 'small' : 'normal' ) . '">%s</div>';

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