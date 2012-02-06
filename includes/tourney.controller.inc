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
    $matches = $this->match_style->build();
    dpm($matches);
    
    // Create the actual match entities and the first game.
  }
}

/**
 * Default interface for any classes that will define a match style.
 */
interface TourneyMatch {
  /**
   * Builds the array structure for all matches to be created.
   */
  public function build();
}
