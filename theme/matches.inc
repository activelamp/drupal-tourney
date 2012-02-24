<?php

// These constants just for easy modifying should the renders need to be resized
const TOURNEY_WIDTH = 120;
const TOURNEY_HEIGHT = 20;

/**
 * This function does the heavy lifting, adding inline css based on what round
 * the contestant is in.
 * 
 * @param $matches
 *   The matches array keyed by round.
 * @param $reset
 *   Boolean to reset the static variables set.
 * @return
 *   Returns the rendered html to be output.
 */
function tourney_render_top_bracket($matches, $reset = false) {

}

/**
 * Makes it easier to render the inline styles
 * 
 * @param $attributes
 *   Associative array with property/value pairs.
 * @return
 *   Returns the compiled HTML attribute and value string.
 */
function tourney_render_style($attributes) {
  $t = '';
  foreach ( $attributes as $k => $v ) $t .= " $k: $v;";
  return " style=\"$t\"";
}

/**
 * Based on bracket type, applies different inline styling
 * 
 * @param $bracket
 *   Bracket type, without bracket-, in string.
 * @param $items
 *   Full tournament, used to get information from other brackets if necessary.
 * @return
 *   Returns the rendered HTML style to be used for the current bracket.
 */
function tourney_bracket_style($bracket, $items) {  
  $attributes = array();
  if ( array_key_exists('bracket-bottom', $items) ) {
    $brounds = count($items['bracket-bottom']);
    $trounds = count($items['bracket-top']);
    if ( $bracket == 'top' && $brounds > $trounds ) $attributes = array(
      'padding-left' => TOURNEY_WIDTH * ( $brounds - $trounds ) . 'px');
    if ( $bracket == 'champion' ) {
      $attributes = array(
        'margin-top' => pow(2, floor($brounds/2)-1)*-TOURNEY_WIDTH-TOURNEY_HEIGHT .'px',
      );
    }
  }
  return tourney_render_style($attributes);
}

/**
 * Based on round number and bracket type, applies different inline styling
 * 
 * @param $round
 *   Integer for which round is currently being rendered.
 * @param $bracket
 *   Bracket the round is in, to determine styling changes.
 * @param $rounds
 *   Integer of relevant round total.
 * @return
 *   Returns the rendered HTML style to be used for the current round.
 */
function tourney_round_style($round, $bracket = '', $rounds = 0) { 
  $er = floor($round/2);
  switch ( $bracket ) {
    case 'bottom':  
      if ( $round % 2 ) $adjust = 0;
      else {
        //$adjust = 30 * ( pow(2, $er+1) - 2 - pow(2, ($er-1)) * (($round-1)%2) );
        $adjust = -30 * pow(2,$er-1); 
      }
      break;
    case 'champion':
      $adjust = pow(2, floor($rounds/2)-1)*-TOURNEY_WIDTH-TOURNEY_HEIGHT;
      if ( $round % 2 == 0 ) $adjust /= 2;
      break;
    default:
    $adjust = pow(2, $round) * ( TOURNEY_HEIGHT / 2 );
  }
  $attributes = array(
    'margin-top' => $adjust . 'px',
  );
  return tourney_render_style($attributes);
}

/**
 * Based on round and bracket types, applies different inline styling to a match
 * 
 * @param $round
 *   Integer for which round is currently being rendered.
 * @param $bracket
 *   Bracket the round is in, to determine styling changes.
 * @param $rounds
 *   Integer of relevant round total.
 * @return
 *   Returns the rendered HTML style to be used for the current match.
 */
function tourney_match_style($round, $bracket = '', $rounds = 0) {  
  switch ( $bracket ) {
    case 'top':
    case 'none':
      $adjust = ( ( pow(2, $round - 1) ) * TOURNEY_HEIGHT - ( TOURNEY_HEIGHT / 2 ) ) * 2;
      break;
    case 'bottom':
      $er = floor(($round-1) / 2);
      $adjust = (35 * pow($er, 2)) + (25 * $er) + 40;
      //if ( $bracket == 'bottom' && $last ) $padding = ($padding * 2.3587) + 40;
      break;
    case 'champion':
      $adjust = pow(2,floor($rounds/2)-1)*TOURNEY_WIDTH;
      break;
  }
  
  $attributes = array(
    'padding-top' =>  $adjust . 'px',
    'margin-bottom' => $adjust . 'px',
  );
  return tourney_render_style($attributes);
}

/**
 * Renders a tournament to HTML and inline CSS
 * 
 * @param $items
 *   Keyed and nested array of all the matches.
 * @return
 *   Returns the rendered HTML for the entire tournament.
 */
function tourney_render_tournament($items) {
  static $round = 0;
  static $rounds = 0;
  static $bracket = 'none';
  $output = '';
  foreach ( $items as $key => $item ) {
    $v = substr($key, 1 + $dash = strpos($key, '-'));
    $k = substr($key, 0, $dash ? $dash : strlen($key));
    $content = '';
    switch ( $k ) {
      case 'bracket':
        $bracket = $v;
        $rounds = count($item);
        if ( $v == 'champion' ) $rounds = count($items['bracket-bottom']);
        $prefix = "<div class=\"bracket $key\" " . tourney_bracket_style($v, $items) . ">";
        $suffix = "<div class='clear'></div></div>";
        break;
      case 'round':
        $round  = $v;
        $style  = tourney_round_style($round, $bracket, $rounds);
        $prefix = "<ul class=\"round $key\" $style>";
        $suffix = "</ul>";
        break;
      case 'match':
        $contestant1 = is_object($item->contestant1) ? $item->contestant1->name : 'team 1';
        $contestant2 = is_object($item->contestant2) ? $item->contestant2->name : 'team 2';
        $title = $item->title .' '. $item->id;
        $c1 = "<span class=\"contestant\">$contestant1</span>";
        $c2 = "<span class=\"contestant last\"" . tourney_match_style($round, $bracket, $rounds) . ">$contestant2</span>";
        $prefix = "<li class=\"match\">";
        $prefix .= theme('ctools_dropdown', array('title' => 'Edit', 'links' => _tourney_format_match_games($items['games'])));
        $content = "$c1 $c2";
        $suffix = "</li>";
        break;
      default:
        $prefix = '';
        $suffix = '';
    }
    if ( $content == '' && is_array($item) ) $content = tourney_render_tournament($item);
    $output .= "$prefix $content $suffix";
  }
  return $output;
}

/**
 * Format an array of game objects to be usable with theme_links.
 * 
 * @param $games
 *   An array of game objects to format.
 */
function _tourney_format_match_games($games) {
  $first_game = reset($games);
  
  $return = array();
  $return[] = array(
    'title' => t('Edit @match', array('@match' => $first_game->match->title)),
    'href' => 'tourney/match/'. $first_game->match->id .'/edit',
    'query' => drupal_get_destination(),
  );
  foreach ($games as $game) {
    $return[] = array(
      'title' => t('Edit @game', array('@game' => $game->title)), 
      'href' => 'tourney/game/'. $game->id .'/edit',
      'query' => drupal_get_destination(),
    );
  }
  return $return;
}

/**
 * Theme implementation for rendering a matches array into brackets.
 */
function theme_tourney_render_matches($vars) {
  $matches = $vars['matches'];
  $output  = "<div class=\"tournament\">";
  $output .= tourney_render_tournament($matches);
  $output .= "</div>";
  drupal_add_css(drupal_get_path('module', 'tourney') . '/theme/bracket.css');  
  return $output;
}
