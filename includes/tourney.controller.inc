<?php

/**
 * @file
 * Provides a controller to build matches after a tournament entity is created.
 */

// Maximum number of rounds that an elimination tournament will allow.
define('MAXIMUM_ROUNDS', 5);
 
/**
 * A Default controller for building matches.
 */
class TourneyBuildMatches {
  
  protected $num_contestants;
  protected $match_style;
  protected $matches;
  
  /**
   * Constructor
   */
  public function __construct($num_contestants, $match_style) {
    $this->num_contestants = $num_contestants;
    if (!class_exists($match_style)) {
      trigger_error(t('Unable to create !match_style matches. Class not found', array('!match_style' => $match_style)), E_USER_ERROR);
    }
    $this->match_style = new $match_style($num_contestants);
  }
  
  /**
   * Create the match entities for the tournament.
   */
  public function create() {
    drupal_set_message('The matches were created');

    // Build the match structure
    $this->matches = $this->match_style->build();

    
    // Create the actual match entities and the first game.
    $this->save();
  }
  
  public function save() {
    $this->save_match($this->matches);
  }
  
  private function save_match($matches) {
    static $round;
    static $bracket;
    
    $identifiers = array('bracket', 'round');
    foreach ($matches as $key => $value) {
      foreach ($identifiers as $identifier) {
        // We only want matches, but keep metadata.
        if (strpos($key, $identifier .'-') === 0 && is_array($value)) {
          ${$identifier} = $key;
        }
      }
      if (strpos($key, 'match-') !== 0 && is_array($value)) {
       $this->save_match($value);
      }
      else {
        $match = $key;
        dpm($value);
      }
      // if (strpos($key, $identifier .'-') !== 0) {
      
    }
  }
  
  private function create_relation($source_entity, $source_id, $destination_entity, $destination_id) {
    $predicate = 'has_match';
    $entity_keys = array(
      array('entity_type' => 'tourney_tournament', 'entity_id' => 1),
      array('entity_type' => 'tourney_match', 'entity_id' => 7),
    );
    $rid = relation_insert($predicate, $entity_keys);
  }
}

/**
 * Default interface for any classes that will define a match style.
 */
interface TourneyMatchInterface {
  /**
   * Builds the array structure for all matches to be created.
   */
  public function build();
}
