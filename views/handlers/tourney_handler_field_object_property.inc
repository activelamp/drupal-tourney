<?php
/**
 * @file
 * Tourney local object property handler
 */
 
/**
 * Provides a new field in views that simply loads the object from views and
 * returns a property from the fully loaded object.
 */
class tourney_handler_field_object_property extends views_handler_field {
  /**
   * We aren't adding anything to the query. Leave blank.
   */
  function query() {}
    
  /**
   * Render the property
   */
  function render(&$values) {
    $match = tourney_match_load($values->id);
    $property = $this->underscoreToCamelCase($this->real_field);
    return is_array($match->$property) ? join(", ", $match->$property) : $match->$property;
  }
  
  /**
   * Replace underscores in string with a camelCase representation.
   */
  function underscoreToCamelCase($str) {
    $parts = explode('_', $this->real_field);
    foreach ($parts as &$part) {
      $part = ucfirst($part);
    }
    return lcfirst(join("", $parts));
  }

}