<?php
function _tourney_render_matches($matches) {
  static $output = '';
  
  foreach ($matches as $key => $value) {
    $entity = '';
    if (is_array($value)) {
      // If the key is an integer, assume it to be an entity_id
      if (is_object($value)) {
        $output .= "<li>". l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
      }
      else {
        $output .= "<li>$key</li>";
      }
      $output .= "<ul>";
      _tourney_render_matches($value);
      $output .= "</ul>";
    }
    else {
      if (is_object($value)) {
        $output .= "<li>". l($value->title, 'tourney/'. $value->type .'/'. $value->id) ."</li>";
      }
      else {
        $output .= "<li>$key</li>";
      }
    }
  }
  return $output;
}

function theme_tourney_render_matches($vars) {
  $matches = $vars['matches'];
  $output = "<ul>". _tourney_render_matches($matches) ."</ul>";
  return $output;
}