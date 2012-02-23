<?php
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
  static $output = '';
  static $ul_top = 0;
  static $team2_top = 0;
  static $previous_round = '';
  static $mar_top = 0;
  static $r = 0;
  static $m = 0;
  static $tm =  0;
  
  // Reset all the static variables.
  if ($reset) {
    $output = '';
    $ul_top = 0;
    $team2_top = 0;
    $previous_round = '';
    $mar_top = 0;
    $r = 0;
    $m = 0;
    $tm =  0;
  }

  foreach ($matches as $key => $value) {
    $entity = '';
    if (is_array($value)) {
      if (strpos($key, 'round-') === 0) {
        // For each round adding a top margin after the first round.
        if ($r == 0) {
          $output .= "<div class='round' style='margin-top:0px;'>";
          $mar_top = 40;
        }
        else {
          $output .= "<div class='round round-" . $r . "' style='margin-top:" . $mar_top . "px;'>";
          $mar_top *= 2;
        }  
        tourney_render_top_bracket($value);
        $output .= "</div>";

        $r++;
        $m = 0;
      }
      else {
        // We are working with an individual match. The array structure we have
        // only has one numeric key with an array as a value.
        if (is_numeric($key)) {
          // For each match, add a margin top. 
          // Skipping the first round and each first match of each round.
          if ($r == 0 || $m == 0)  {
            $output .= "<ul style='margin-top:0px;'>";            
            $ul_top = ($tm == 0) ? 30 : $ul_top;
          }
          else {
            // Only change the margin if we have moved on to another round.
            if ($r != $previous_round) {
              $ul_top *= 2;
              // The margin needs to add 20px on every round except the 1st.
              if ($r > 1) {
                $ul_top += 20;
              }
            }
            $previous_round = $r;
            $output .= "<ul style='margin-top:" . $ul_top . "px;'>";
          }
        }
        // This is a game. Create the wrapper.
        else {
          $output .= "<ul class='games'>";
        }
        
        // Recursive function to find an object and render the match or game.
        tourney_render_top_bracket($value);
        
        $output .= "</ul>";
        
        // Match iterator and Total Match iterator increased by 1
        $m++;
        $tm++;
      }
    }
    
    // We have an object, output the contestants if it's a match, or output a
    // list of games.
    else {
      if (is_object($value) && $value->type == 'match') { 
        $contestant1 = is_object($value->contestant1) ? $value->contestant1->name : '';
        $contestant2 = is_object($value->contestant2) ? $value->contestant2->name : '';
        // If not on the first round, do not add a padding-top. All team names
        // in the first round.
        if ($r == 0) {
          $output .= "<li>". $contestant1 .' '. l($value->title .' '. $value->id, 'tourney/'. $value->type .'/'. $value->id .'/edit', array(), drupal_get_destination()) ."</li>";
          $output .= "<li>". $contestant2 .' '. l($value->title .' '. $value->id, 'tourney/'. $value->type .'/'. $value->id .'/edit', array(), drupal_get_destination()) ."</li>";
          $output .= theme('ctools_dropdown', array('title' => 'Games', 'links' => _tourney_format_match_games($matches['games'])));
          $team2_top = ($tm == 0) ? 30 : $team2_top;
        }
        else {
          // If this is the same round, add the same amount of padding-top.
          if ($r != $previous_round) {
            $team2_top *= 2;
            // The padding needs to add 20px on every team except the 1st.
            if ($r > 1) {
              $team2_top += 20;
            }
          }
          // All team names besides the 1st round.
          $output .= "<li style='padding-top:0px;'>". $contestant1 .' '. l($value->title .' '. $value->id, 'tourney/'. $value->type .'/'. $value->id .'/edit', array(), drupal_get_destination()) ."</li>";
          $output .= "<li style='padding-top:" . $team2_top . "px;'>". $contestant2 .' '. l($value->title .' '. $value->id, 'tourney/'. $value->type .'/'. $value->id .'/edit', array(), drupal_get_destination()) ."</li>"; 
          $output .= theme('ctools_dropdown', array('title' => 'Games', 'links' => _tourney_format_match_games($matches['games'])));
        }
      }
    }
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
  $return = array();
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
 * Theme implementation for rendering a matches array into a bracket.
 */
function theme_tourney_render_matches($vars) {
  $matches = $vars['matches'];
  dpm($matches);
  $output = '';
  
  // If we have multiple brackets, output them one by one
  $key = array_pop(array_keys($matches));
  if (strpos($key, 'bracket-') === 0) {
    foreach($matches as $bracket_name => $bracket) {
      $name = substr($bracket_name, 8);
      // Default the bracket theme function to top bracket if a default
      // implementation does not exist.
      if (!function_exists('tourney_render_'. $name .'_bracket')) {
        $name = 'top';
      }
      
      $output .= "<div class='tournament'>";
      $output .= call_user_func_array('tourney_render_'. $name .'_bracket', array($bracket, TRUE));
      $output .= "</div>";
    }
  }
  // This is a single bracket
  else {
    $output .= "<div class='tournament'>";
    $output .= call_user_func_array('tourney_render_top_bracket', array($matches, TRUE));
    $output .= "</div>";
  }
  
  drupal_add_css(drupal_get_path('module', 'tourney') . '/theme/bracket.css');
  
  return $output;
}