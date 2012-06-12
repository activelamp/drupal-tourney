<?php

/**
 * @file
 * Single elimination controller, new system.
 */

class SpecialEliminationController extends TourneyController implements TourneyControllerInterface {
  public $slots;
  public $contestants;
  public $data;
  public $structure;

  /**
   * Constructor
   */
  public function __construct($contestants) {
    // Set our contestants, and then calculate the slots necessary to fit them 
    $this->contestants = $contestants;  
    $this->slots = pow(2, ceil(log($contestants, 2)));
    // Build our data structure
    $this->build();
    $this->structure();
  }

  /**
   * Data generator
   */
  public function build() {    
    $slots = $this->slots;
    $match = 0;
    $round = 0;
    // Calculate and iterate through rounds and their matches based on slots
    while ( ($slots /= 2) >= 1 ) {
      // Add current round information to the data array
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round));

      // Add in all matches and their information for this round
      foreach ( range(1, $slots) as $roundMatch ) {
        $this->data['matches'][++$match] = 
          $this->buildMatch(array(
            'id' => $match,
            'round' => $round,
            'roundMatch' => (int) $roundMatch,
          ));
      }
    }
    // Set in the seed positions
    $this->populateSeedPositions();
    // Calculate and set the match pathing
    $this->populatePositions();
  }

  /**
   * Round data generator
   *
   * @param array $data
   *   Uses 'id' from the array to set basic values, and joins for the rest
   *
   * @return $round
   *   Filled out round data array
   */
  public function buildRound($data) {
    $round = array(
      'title' => 'Round ' . $data['id'],
      'id'    => 'round-' . $data['id'],
    ) + $data;
    return $round;
  }

  /**
   * Match data generator
   *
   * @param array $data
   *   Uses 'id' from the array to set basic values, and joins for the rest
   *
   * @return $match
   *   Filled out match data array
   */
  public function buildMatch($data) {
    $match = array(
      'title'   => "Match " . $data['id'],
      'id'      => "match-" . $data['id'],
      'match'   => $data['id'],
    ) + $data;
    return $match;
  }

  /**
   * Find and populate next/previous match pathing
   */
  public function populatePositions() {
    // Go through all the matches
    foreach ( $this->data['matches'] as &$match ) {
      // Find next match by filtering through matches with in the next round
      // and those with a halved round match number
      // Example:
      //   Round 3, Match 5
      //  Next match is:
      //    Round 4 [3+1], Match 3 [ceil(5/2)]
      $next = $this->find('matches', array(
        'round' => $match['round'] + 1,
        'roundMatch' => (int) ceil($match['roundMatch'] / 2),
      ), 'id');
      // If find()'s returned a result, set it.
      if ( $next ) $match['nextMatch']['winner'] = array_pop($next);

      // Target is multiplied by two to count for the /2 we used in nextMatch
      // Example:
      //   Round 3, Match 5
      //  Previous matches:
      //    Round 2 [3-1], Match 10 [5*2]
      //    Round 2 [3-1], Match 9 [5*2-1]
      $target = $match['roundMatch'] * 2;
      $prev = $this->find('matches', array(
        'round' => $match['round'] - 1,
        'roundMatch' => array($target, $target - 1),
      ), 'id');
      // If find()'s returned a result, set it.
      if ( $prev ) $match['previousMatches'] = $prev;
    }
  }

  /**
   * Calculate and fill seed data into matches
   */
  public function populateSeedPositions() {
    $this->calculateSeeds();
    // Calculate the seed positions, then apply them to their matches while
    // also setting the bye boolean
    foreach ( $this->data['seeds'] as $id => $seed ) {
      $match =& $this->data['matches'][$id];
      $match['seeds'] = $seed;
      $match['bye']   = $seed[1] === NULL;
    }
  }

  /**
   * Find elements given specific information
   *
   * @param string $data
   *   Data element from $this->data to search
   *
   * @param array $vars
   *   Keyed array of values on the elements to filter
   *   If one of the variables is an array, it will compare the testing
   *     element's value against each of the array's 
   *
   * @param string $specific
   *   Single value from each element to return, if not given will return
   *   the full element
   *
   * @param boolean $first
   *   If TRUE, will return the first matched element
   *
   * @return $elements
   *   Array of elements that match the $vars given
   */
  public function find($data, $vars, $specific = NULL, $first = FALSE) {
    if ( !array_key_exists($data, $this->data) ) return NULL;
    $elements = array();
    // is_array is expensive, set up an array to store this information
    $is_array = array();
    foreach ( $vars as $key => $value )
      $is_array[$key] = is_array($value);
    // Loop through all elements of the requested data array 
    foreach ( $this->data[$data] as $id => $element ) {
      // Compare all our required $vars with its applicable properties
      // If that specific $vars is an array, check to see if the element's 
      // property is in the array
      // If the element fails at any of the checks, skip over it
      foreach ( $vars as $key => $value ) {
        if ( $element[$key] !== $value ) {
          if ( !$is_array[$key] || !in_array($element[$key], $value) ) 
            continue 2;
        }
      }
      // If we've supplied a 'specific' argument, only take that value,
      // otherwise take the entire element
      if ( $specific !== NULL )
        $elements[] = $element[$specific];
      else 
        $elements[] = $element;
      // When $first, don't go any further once the first element has been set
      if ( $first === TRUE ) break;
    }
    return $elements;
  }

  /**
   * Generate a structure based on data
   */
  public function structure() {
    $structre = array();
    // Loop through our rounds and set up each one
    foreach ( $this->data['rounds'] as $round ) {
      $structure[$round['id']] = $round + array('matches' => array());
    }
    // Loop through our matches and add each one to its related round
    foreach ( $this->data['matches'] as $match ) {
      $structure['round-' . $match['round']]['matches'][$match['id']] = $match;
    }
    $this->structure = $structure;
  }

  public function determineWinner($tournament) {

  }

  public function isFinished($tournament) {

  }

  /**
   * Calculate contestant starting positions
   */
  public function calculateSeeds() {
    // Set up the first seed position
    $seeds = array(1);
    // Setting a count variable lets us speed up execution by not calling it
    // several times each iteration
    $count = 0;
    // Keep generating the series until we've met our number of slots
    while ( ($count = count($seeds)) < $this->slots ) {
      $new_seeds = array();
      // For every current seed number, we'll add in one after it that
      // matches the current from the end of the new count.
      // Example:
      // (1, 2)
      //   The new series will have 4 elements, which is the current * 2
      // 4 - 1 = 3, however because we're 1-based, add 1: 4
      //  (1, 4)
      // 4 - 2 = 2 + 1 = 3
      //  (2, 3)
      // New series:
      // (1, 4, 2, 3)
      foreach ( $seeds as $seed ) {
        $new_seeds[] = $seed;
        $new_seeds[] = ($count * 2 + 1) - $seed;
      }
      // Set these changes to be iterated through again if necessary
      $seeds = $new_seeds;
    }
    // Now that we've generated a full list of positions, fill them out into
    // a series of matches with two positions per.
    // Loop through each two of them, and fill out all the seeds as long as
    // they're within the number of participating contestants.
    $positions = array();
    for ( $p = 0; $p < $count; $p += 2 ) {
      $a = $seeds[$p];
      $b = $seeds[$p+1];
      $positions[] = array($a, $b <= $this->contestants ? $b : NULL);
    }
    // This logic changed our zero-based array to one-based so our match
    // ids will line up
    array_unshift($positions, NULL);
    unset($positions[0]);
    $this->data['seeds'] = $positions;
  }
}