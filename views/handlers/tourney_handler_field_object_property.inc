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
    $entity = $this->load($values->id);
    $property = $this->underscoreToCamelCase($this->real_field);
    return is_array($entity->$property) ? join(", ", $entity->$property) : $entity->$property;
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
  
  /**
   * Load the entity for the type of tourney entity this is. Uses the primary
   * table to determine the type of entity.
   * 
   * @param $id
   *   The entity id to load.
   * @return
   *   Returns the fully loaded object.
   */
   function load($id) {
     @list(, $type) = explode("_", $this->table);
     $function = 'tourney_' . $type . '_load';
     return $function($id);
   }
}