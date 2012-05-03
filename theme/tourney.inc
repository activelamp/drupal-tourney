<?php

/*
 * @file
 */

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
  $format = $types[$tournament->format];
  if (array_key_exists('matches', $vars)) {
    $matches = $vars['matches'];
  }
  else {
    $matches = $tournament->buildMatches();
  }

  $output = "<div class='tourney tourney-$format'><div class='tourney-inner'>";
  $output .= theme("tourney_render_tourney_$format", array('tournament' => $tournament, 'matches' => $matches));
  $output .= "</div></div>";
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
function theme_tourney_flow_control($vars) {
  $flows = $vars['flows'];
  if ( !$flows ) return;
  $attributes = (!empty($options['attributes']) ? $options['attributes'] : array());
  $attributes['class'][] = 'flow';

  $output = '<div ' . drupal_attributes($attributes) . '><div class="flow-control '
        . implode('"></div><div class="flow-control ', explode(' ', $flows))
        . '"></div></div>';
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
function theme_tourney_matchblock($vars) {
  extract($vars);
  $output  = '<div class="match ' . ( $small ? 'small' : 'normal') . '">';
  $output .= theme('tourney_flow_control', array('flows' => $flow));

  $cid = 1;

  $contestants = $match->getContestants();
  while ( count($contestants) < 2 ) $contestants[] = NULL;
  $editable = in_array(NULL, $contestants);
  foreach ( $contestants as $eid => $contestant ) {
    if ( $contestant == NULL ) {
      $output .= '<div class="contestant contestant-' . $cid . '"> Team ' . $cid++ . ' </div>';
    }
    else {
      $output .= '<div class="contestant contestant-' . $cid++ . ' entity-' . str_replace('.', '-', $eid) . '">' 
        . $contestant->entity_name . ( $eid == $match->winner ? ' ♔' : '' ) 
        . '</div>';
    }
  }
  $output .= '</div>';

  return l($output, "tourney/match/{$match->id}" . ($editable ? "/edit" : ''), array(
    'html' => TRUE,
    'attributes' => array('class' => 'match-link'),
    'query' => $editable ? array('destination' => "tourney/match/{$match->id}") : array()
  ));
}

/**
 * Helper function to render spacing divs
 *
 * @param $height
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
function theme_tourney_spacer($vars) {
  extract($vars);
  $div = '<div class="match match-spacer ' . ( $small ? 'small' : 'normal' ) . '">%s</div>';
  if ( $height <= 0 ) return;
  if ( $flow ) {
    $half    = floor($height/2);
    $normal  = sprintf($div, theme('tourney_flow_control', array('flows' => 'vertical')));
    $output  = $y == 0 ? sprintf($div, '') . str_repeat($normal, $half - 1 ) : str_repeat($normal, $half);
    $output .= sprintf($div, is_string($flow) ? 
      theme('tourney_flow_control', array('flows' => $flow)) : 
      ( $height % 2 ? 
        theme('tourney_flow_control', array('flows' => 'topright bottomright')) : 
        theme('tourney_flow_control', array('flows' => 'evenright')) ));
    if ( !($height % 2) ) $half--;
    $output .= str_repeat($normal, $half);
  }
  else
    $output  = str_repeat(sprintf($div, ''), $height);
  return $output;
}

/**
 * Theme the match edit table of players
 */
function theme_tourney_match_fix($vars) {
  $element = $vars['element'];
  
  // Get the keys of the game form elements in this match render element
  // @see tourney_match_form().
  $children = element_children($element);
  
  // Initialize the header and rows
  $header = array('Team Name');
  $rows   = array();
  
  // Loop through all the games that have started.
  for ($gn = 0; $gn < $element['#games']; $gn++) {
    $header[] = "Game " . ( $gn + 1 );
    
    if ($gn < count($children)) {
      // Get the form element for this game number.
      $game = $element[$children[$gn]];
      foreach (element_children($game) as $user) {
        if (!isset($rows[$user])) {
          $rows[$user][] = $game[$user]['#title'];
        }
        // Render the form element as a column in this row of the table.
        $rows[$user][] = drupal_render($game[$user]);
      }
    }
    else {
      foreach ($rows as $id => &$row) {
        $row[] = ' - ';
      } 
    }
  }

  $header[] = "Results";
  foreach ( $rows as $id => &$row ) {
    $row[] = ' - ';
  }

  return theme('table', array(
    'attributes' => array('id' => 'game-list'),
    'header' => $header,
    'rows' => $rows,
  ));
}