<?php

/*
 * @file
 * Holds the theme implementations for rendering tournaments
 */

/**
 * Theme function to return HTML output for a given tournament, delegates the
 * rendering to the tourney format plugin.
 *
 * @param $vars
 *   Should be a single-key array containing the tournament
 * @return
 *   Tree in HTML output
 */
function theme_tourney_render_tourney($vars) {
  return $vars['tournament']->render();
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
  static $debug_val = 0;
  $output  = '<div class="match ' . ( $small ? 'small' : 'normal') . '">';
  $output .= theme('tourney_flow_control', array('flows' => $flow));

  $cid = 1;

  $contestants = $match->getContestants();
  // If there is one contestants, pad the beginning if it's occupying slot 2
  if ( count($contestants) == 1 ) {
    $first = current($contestants);
    if ( $first->slot == 2 ) array_unshift($contestants, NULL);
  }
  while ( count($contestants) < 2 ) $contestants[] = NULL;
  $editable = in_array(NULL, $contestants) && $match->canSetContestants();
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

  // $output = "<div class='match normal'>" . theme('tourney_flow_control', array('flows' => $flow)) 
  //    . '<div class="contestant contestant-1">' . ( $match->nextMatch(1) ? $match->nextMatch(1)->entity_id 
  //     . '.' . $match->getTournament()->tourneyFormatPlugin->getNextSlot($match, 1) : 'END' ) . '</div>'
  //    . '<div class="contestant contestant-2">' . ( $match->nextMatch(2) ? $match->nextMatch(2)->entity_id 
  //     . '.' . $match->getTournament()->tourneyFormatPlugin->getNextSlot($match, 2) : 'END' ) . '</div>'
  //    . '</div>';
  
  $match_uri = $match->getUri();
  return l($output, $match_uri . ($editable ? "/edit" : ''), array(
    'html' => TRUE,
    'attributes' => array('class' => 'match-link'),
    'query' => $editable ? array('destination' => $match_uri) : array()
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