<?php

/*
 * @file
 * Preprocess functions for all theme implementations provided by tourney.
 */
 
/**
 * 
 */
function template_preprocess_tourney_tournament_render(&$vars) {
  $plugin = &$vars['format_plugin'];
  
  drupal_add_css(drupal_get_path('module', 'tourney') . '/theme/tourney.css');
  // Let the plugin preprocess variables
  $plugin->preprocess('tourney-tournament-render', $vars);
  
  // Build the tourney container classes.
  $vars['classes'] = join(' ', $vars['classes_array']);
  
  // Call on the theme function specified by plugin render
  $vars['matches'] = theme($vars['theme'], array('format_plugin' => $plugin));
}

/**
 * 
 */
function template_preprocess_tourney_match_render(&$vars) {
  $mid = $vars['match']['id'];
  $plugin = &$vars['format_plugin'];
  $editable = FALSE;
  
  // Let the plugin preprocess variables
  $plugin->preprocess('tourney-match-render', $vars);
  
  $contestants = $plugin->getMatchContestants($mid);
  // If there is one contestants, pad the beginning if it's occupying slot 2
  if (count($contestants) == 1) {
    $first = current($contestants);
    if ($first->slot == 2) {
      array_unshift($contestants, NULL);
    }
  }
  // Fill in null contestants if two contestants are not assigned to match.
  $i = 0;
  while (count($contestants) < 2) {
    $contestants[++$i] = (object) array('name' => NULL, 'id' => NULL);
    // This match is editable when at least one of the contestants is NULL.
    $editable = TRUE;
  }
  
  // Get the contestant output from the contestant template
  $contestant_output = '';
  foreach ($contestants as $slot => $contestant) {
    $contestant->slot = $slot;
    $contestant_output .= theme('tourney_contestant', array(
      'contestant' => $contestant, 
      'seed' => isset($seeds[$slot]) ? $seeds[$slot] : NULL,
      'format_plugin' => $plugin,
    ));
  }
  
  // If we have a tournament object, get the match url from the plugin.
  $tournament = $plugin->tournament;
  if (is_object($tournament) && get_class($tournament) == 'TourneyTournamentEntity') {
    $match_uri = $plugin->getMatchURI($mid);
    if ($match_uri) {
      $editable = $editable == TRUE && $plugin->canSetContestants($mid);
      $contestant_output = l($contestant_output, $match_uri . ($editable ? "/edit" : ''), array(
        'html' => TRUE,
        'attributes' => array('class' => 'match-link'),
        'query' => $editable ? array('destination' => $match_uri) : array(),
      ));
    }
  }
  
  $vars['contestants'] = $contestant_output;
}

/**
 * 
 */
function template_preprocess_tourney_contestant(&$vars) {
  $plugin = &$vars['format_plugin'];
  
  // Let the plugin preprocess variables
  $plugin->preprocess('tourney-contestant', $vars);
  
  if (!$vars['contestant']->name ) {
    $vars['contestant']->name = 'Team ' . $vars['contestant']->slot;
  }
}
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

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
  if ( !$match ) return;
  static $debug_val = 0;

  $is_bye = array_key_exists('bye', $match->matchInfo) && $match->matchInfo['bye'];
  // Build a classes array for the match wrapper
  $classes_array[] = 'match';
  $classes_array[] = ($is_bye ? 'match-bye' : '');
  $classes_array[] = ($small ? 'small' : 'normal');
  
  $output  = '<div class="' . join(" ", $classes_array) . '">';
  $output .= theme('tourney_flow_control', array('flows' => $flow));

  $cid = 1;

  $contestants = $match->getMatchContestants();
  // If there is one contestants, pad the beginning if it's occupying slot 2
  if (count($contestants) == 1) {
    $first = current($contestants);
    if ($first->slot == 2) {
      array_unshift($contestants, NULL);
    }
  }
  while (count($contestants) < 2) {
    $contestants[] = NULL;
  }
  
  $editable = in_array(NULL, $contestants) && $match->canSetContestants();
  
  foreach ($contestants as $eid => $contestant) {
    $classes_array = array('contestant');
    if ($contestant == NULL) {
      $classes_array[] = 'contestant-' . $cid;
      $team_name = 'Team ' . $cid++;
    }
    else {
      if ( $eid == $match->winner )
        $classes_array[] = 'winner';
      $classes_array[] = 'contestant-' . $cid++;
      $classes_array[] = 'entity-' . str_replace('.', '-', $eid);
      if ( $eid == $match->winner && $match->getTournament()->get('show_crowns') )  
        $team_name = $contestant->entity_name . ' ♔';
      else
        $team_name = $contestant->entity_name;  
    }
    // If this is a bye blank out second contestant.
    if ($is_bye && $cid == 3) {
      $classes_array[] = 'contestant-bye';
      $team_name = '';
    }
    
    $s = $cid - 2;
    $output .= '<div class="' . join(" ", $classes_array) . '">'; 
    
    // Set seed info from bye
    if ( $match->getTournament()->get('show_byes') 
      && !empty($match->matchInfo['round']['id']) 
      && $match->matchInfo['round']['id'] == 2 ) {
      $match->findSeedInfo($s);
    }    
    
    if (!empty($match->matchInfo['seed_position'][$s])) {
      $output .= '<span class="seed">' . $match->matchInfo['seed_position'][$s] . '</span>';
    }
    $output .= $team_name;
    $output .= '</div>';
  }
  $output .= '</div>';
  
  /**
   * Debug code for testing match paths. Useful for making sure matches are feeding
   * into the proper position.
   */
  if ($match->getTournament()->get('show_next_match_path')) {
    $output = "<div class='match normal'>" . theme('tourney_flow_control', array('flows' => $flow)) 
      . '<div class="contestant contestant-1">' . $match->entity_id . '</div>'
      . '<div class="contestant contestant-2">' 
      . '<span class="label">W:</span>' . ( $match->nextMatch() ? $match->nextMatch()->entity_id : 'NULL' ) . ' . ' 
      . '<span class="label">L:</span>' . ( $match->nextMatch('loser') ? $match->nextMatch('loser')->entity_id : (array_key_exists('bye', $match->matchInfo) ? 'BYE' : '-') ) . '</div>'
      . '</div>';
  }
  elseif ($match->getTournament()->get('show_previous_match_path')) {
    $previousMatches = $match->previousMatches();
    $output = "<div class='match normal'>" . theme('tourney_flow_control', array('flows' => $flow)) 
      . '<div class="contestant contestant-1">' . $match->entity_id . '</div>'
      . '<div class="contestant contestant-2">' 
      . '<span class="label">1:</span>' . ( array_key_exists(0, $previousMatches) ? $previousMatches[0]->entity_id : '-' ) . ' . ' 
      . '<span class="label">2:</span>' . ( array_key_exists(1, $previousMatches) ? $previousMatches[1]->entity_id : '-' ) . '</div>'
      . '</div>';
  }
  /** End of debug code */

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