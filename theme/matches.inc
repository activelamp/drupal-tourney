<?php
function _tourney_render_matches($matches) {
  static $output = '';
  static $ul_top = 0;
  static $team1_top = 0;
  static $team2_top = 0;
  static $previous_round;
  
  // Define an iterator for each round and match.
  static $r = 0;
  // Define an iterator for each match within a round. This iterator gets reset
  // after each round.
  static $m = 0;
  // This is the total match count.
  static $tm = 0;
  foreach ($matches as $key => $value) {
    $entity = '';
    if (is_array($value)) {
      // If the key is an integer, assume it to be an entity_id
      if (is_object($value)) {
        $output .= "<li>". l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
      }
      // For each round adding a top margin after the first round.
      if (strpos($key, 'round-') === 0) {
        if ($r == 0) {
          $output .= "<div class='round' style='margin-top:0px;'>";
          $mar_top = 40;
        }
        else {
          $output .= "<div class='round round-" . $r . "' style='margin-top:" . $mar_top . "px;'>";
          $mar_top *= 2;
        }  
        _tourney_render_matches($value);
        $output .= "</div>";
        $r++;
        $m = 0;
      }
      else {
        // For each set of matches, adding a margin top. 
        if (is_numeric($key)) {
          // Skipping the first round and each first match of each round.
          if ($r == 0 || $m == 0)  {
            $output .= "<ul style='margin-top:0px;'>";            
            $ul_top = ($tm == 0) ? 30 : $ul_top;
          }

          else {
            // If this is in the same round, give the same margin.
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
        // Fallback
        else {
          if (strpos($key, 'bracket-') === 0) {
            
          }
          $output .= "<ul class='game-" . $m . "'>";
        }
        _tourney_render_matches($value);
        $output .= "</ul>";
        $m++;
        $tm++;
      }
    }
    
    // This outputs the contestants.
    else {
      if (is_object($value) && $value->type == 'match') { 
        $contestant1 = is_object($value->contestant1) ? $value->contestant1->name : '';
        $contestant2 = is_object($value->contestant2) ? $value->contestant2->name : '';
        // If not on the first round, do not add a padding-top. All team names
        // in the first round.
        if ($r == 0) {
          $output .= "<li>". $contestant1 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
          $output .= "<li>". $contestant2 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
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
          $output .= "<li style='padding-top:0px;'>". $contestant1 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
          $output .= "<li style='padding-top:" . $team2_top . "px;'>". $contestant2 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>"; 
        }
      }
      // This outputs the game li's.
      elseif ((is_object($value))) {
        $output .= "<li class='team-game'>". l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
      }
    }
  }
  return $output;
}


function _tourney_render_bracket_bottom($matches) {
  static $output = '';
  static $ul_top = 0;
  static $team1_top = 0;
  static $team2_top = 0;
  static $previous_round;
  
  // Define an iterator for each round and match.
  static $r = 0;
  // Define an iterator for each match within a round. This iterator gets reset
  // after each round.
  static $m = 0;
  // This is the total match count.
  static $tm = 0;
  foreach ($matches as $key => $value) {
    $entity = '';
    if (is_array($value)) {
      // If the key is an integer, assume it to be an entity_id
      if (is_object($value)) {
        $output .= "<li>". l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
      }
      // For each round adding a top margin after the first round.
      if (strpos($key, 'round-') === 0) {
        if ($r == 0) {
          die;
          $output .= "<div class='round' style='margin-top:0px;'>";
          $mar_top = 40;
        }
        else {
          $output .= "<div class='round round-" . $r . "' style='margin-top:" . $mar_top . "px;'>";
          $mar_top *= 2;
        }  
        _tourney_render_matches($value);
        $output .= "</div>";
        $r++;
        $m = 0;
        krumo($m);
      }
      else {
        // For each set of matches, adding a margin top. 
        if (is_numeric($key)) {
          // Skipping the first round and each first match of each round.
          if ($r == 0 || $m == 0)  {
            $output .= "<ul style='margin-top:0px;'>";            
            $ul_top = ($tm == 0) ? 30 : $ul_top;
          }

          else {
            // If this is in the same round, give the same margin.
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
        // Fallback
        else {
          if (strpos($key, 'bracket-') === 0) {
            
          }
          $output .= "<ul class='game-" . $m . "'>";
        }
        _tourney_render_matches($value);
        $output .= "</ul>";
        $m++;
        $tm++;
      }
    }
    
    // This outputs the contestants.
    else {
      if (is_object($value) && $value->type == 'match') { 
        $contestant1 = is_object($value->contestant1) ? $value->contestant1->name : '';
        $contestant2 = is_object($value->contestant2) ? $value->contestant2->name : '';
        // If not on the first round, do not add a padding-top. All team names
        // in the first round.
        if ($r == 0) {
          $output .= "<li>". $contestant1 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
          $output .= "<li>". $contestant2 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
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
          $output .= "<li style='padding-top:0px;'>". $contestant1 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
          $output .= "<li style='padding-top:" . $team2_top . "px;'>". $contestant2 .' '. l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>"; 
        }
      }
      // This outputs the game li's.
      elseif ((is_object($value))) {
        $output .= "<li class='team-game'>". l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
      }
    }
  }
  return $output;
}

function theme_tourney_render_matches($vars) {
  $matches = $vars['matches'];
  krumo($matches);
  $keys = array_keys($matches);
  $output = '';
  krumo($keys);
  foreach ($keys as $key) {
    if ($key == 'bracket-bottom') { 
      $output .= "<div class='tournament'>" . _tourney_render_bracket_bottom($matches) . "</div>";
    }
    else {
      $output .= "<div class='tournament'>" . _tourney_render_matches($matches) . "</div>";
    }
  }
  drupal_add_css(drupal_get_path('module', 'tourney') . '/theme/bracket.css');
  return $output;
}