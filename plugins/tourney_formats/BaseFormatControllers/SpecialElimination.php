<?php

/**
 * @file
 * Single elimination controller, new system.
 */

class TourneyFormatController {
  public $data;
  public $structure;
  public $contestants;

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
   *   Filled out match object
   */
  public function buildMatch($data) {
    $match = array(
      'controller'  => $this,
      'title'       => 'Match ' . $data['id'],
      'id'          => 'match-' . $data['id'],
      'match'       => $data['id'],
    ) + $data;
    return $data;
  }

  /**
   * Game data generator
   *
   * @param array $data
   *   Uses 'id' from the array to set basic values, and joins for the rest
   *
   * @return $match
   *   Filled out game object
   */
  public function buildGame($data) {
    $game = array(
      'title'       => 'Game ' . $data['game'],
      'id'          => 'game-' . $data['id'],
    ) + $data;
    return $game;
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
   * @param boolean $first
   *   If TRUE, will return the first matched element
   *
   * @param string $specific
   *   Single value from each element to return, if not given will return
   *   the full element
   *
   * @return $elements
   *   Array of elements that match the $vars given
   */
  public function &find($data, $vars, $first = FALSE, $specific = NULL) {
    if ( !isset($this->data[$data]) ) return NULL;
    // Added in optimization to make routine find() calls faster
    // Normally searches are incremented, so this optimization holds
    // the place of the last call and continues the search from there
    //
    // Implementing this speeds calls from a 2048 contestant tournament
    //  up from 8.7 seconds to 2.1 seconds
    //
    // $optimize_data  : stores the last data array searched
    // $optimize_last  : the key left off on the last search
    // $optimize_until : in the case we return no elements in a search we
    //                   used optimization in, retry the search but only
    //                   until this key
    // $optimize_using : is set to determine whether we're optimizing
    //                   even after $optimize_last is cleared
    static $optimize_data  = NULL;
    static $optimize_last  = NULL;
    static $optimize_until = NULL;
           $optimize_using = $optimize_last;
    static $optimize_array = array();

    if ( $optimize_data !== $data ) {
      $optimize_last  = NULL;
      $optimize_until = NULL;
      $optimize_using = NULL;
      $optimize_data  = $data;
    }

    $elements = array();
    // is_array is expensive, set up an array to store this information
    $is_array = array();
    foreach ( $vars as $key => $value )
      $is_array[$key] = is_array($value);
    // Loop through all elements of the requested data array 
    foreach ( $this->data[$data] as $id => &$element ) {
      // We can only really optimize $first queries, since anything other
      // has to loop through all the elements anyways
      if ( $first && $optimize_last ) {
        // Until we hit the key we left off at, keep skipping elements...
        if ( $id !== $optimize_last ) continue;
        // ...and then we clear the variable so we can continue on.
        $optimize_last  = NULL;
      }
      // The other end of this is if we're continuing a failed optimized
      // search to exit out of the loop once we've hit where we started from
      if ( $optimize_until && $id == $optimize_until ) break;
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
        $elements[] = $data_is_array ? $element[$specific] : $element->$specific;
      else 
        $elements[] = &$element;
      // When $first, don't go any further once the first element has been set
      if ( $first === TRUE ) {
        // Set the optimizing static so we know where to start from next time
        $optimize_last = $id;
        return $elements[0];
      }
    } 
    // We're out of the loop, clear the static in case it went through all of 
    // the keys without stopping at one
    $optimize_last = NULL;
    // If we have no elements and we were using optimiziation...
    if ( !$elements && $optimize_using ) {
      // ...set the end key to what we started from
      $optimize_until = $optimize_using;
      $optimize_using = NULL;
      // and search again for
      $elements = $this->find($data, $vars, $first, $specific);
    } 
    return $elements;
  }
}


class SpecialEliminationController extends TourneyFormatController {
  public $slots;

  /**
   * Constructor
   */
  public function __construct($contestants, $tournament = NULL) {
    // Set our contestants, and then calculate the slots necessary to fit them 
    $this->contestants = $contestants;  
    $this->slots = pow(2, ceil(log($contestants, 2)));
    $this->tournament = $tournament;
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
    foreach ( $this->data['matches'] as $id => &$match ) {
      $this->data['games'][$id] = $this->buildGame(array(
        'id' => $id,
        'match' => $id,
        'game' => 1, 
      ));
      $this->data['matches'][$id]['games'][] = $id;
    }
    $this->data['contestants'] = array();
    // Calculate and set the match pathing
    $this->populatePositions();
    // Set in the seed positions
    $this->populateSeedPositions();
  }


  /**
   * Find and populate next/previous match pathing
   */
  public function populatePositions() {
    // Go through all the matches
    $count = count($this->data['matches']);
    foreach ( $this->data['matches'] as $id => &$match ) {
      if ( $id == $count ) continue;
      // Find next match by filtering through matches with in the next round
      // and those with a halved round match number
      // Example:
      //   Round 3, Match 5
      //  Next match is:
      //    Round 4 [3+1], Match 3 [ceil(5/2)]

      $next = &$this->find('matches', array(
        'round' => $match['round'] + 1,
        'roundMatch' => (int) ceil($match['roundMatch'] / 2),
      ), TRUE);

      // $index = ( $this->slots / 2 ) + floor(($match->match-1) / 2)+1;
      // $next = $this->data['matches'][$index];

      // If find()'s returned a result, set it.
      if ( $next ) {
        $match['nextMatch']['winner'] = $next['id'];
        $next['previousMatches'][] = $match['id'];
      } 
    }
  }

  /**
   * Calculate and fill seed data into matches
   */
  public function populateSeedPositions() {
    $this->calculateSeeds();
    // Calculate the seed positions, then apply them to their matches while
    // also setting the bye boolean
    foreach ( $this->data['seeds'] as $id => $seeds ) {
      $match =& $this->data['matches'][$id];
      $match['seeds'] = $seeds;
      $match['bye']   = $seeds[2] === NULL;
      if ( $match['bye'] && isset($match['nextMatch']) )
        $this->data['matches'][$match['nextMatch']['winner']]['seeds'] = 
          array(1 => $seeds[1], 2 => NULL);
    }
  }

  /**
   * Generate a structure based on data
   */
  public function structure($type = 'nested') {
    switch ( $type ) {
      case 'nested':
        $this->structure['nested'] = $this->structureNested();
        break;
      case 'tree':
        $this->structure['tree'] = $this->structureTree();
        break;
    }
    return $this->structure[$type];
  }

  public function structureNested() {
    $structure = array();
    // Loop through our rounds and set up each one
    foreach ( $this->data['rounds'] as $round ) {
      $structure[$round['id']] = $round + array('matches' => array());
    }
    // Loop through our matches and add each one to its related round
    foreach ( $this->data['matches'] as $match ) {
      $structure['round-' . $match['round']]['matches'][$match['id']] = $match;
    }
    return $structure;
  }

  public function structureTree() {
    $match = end($this->data['matches']);
    return $this->structureTreeNode($match);
  }

  public function structureTreeNode($match) {
    $node = $match;
    if ( isset($match['previousMatches']) )
      foreach ( $match['previousMatches'] as $child )
        $node['children'][] = $this->structureTreeNode($this->data['matches'][$child]);
    return $node;
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
      $positions[] = array(1 => $a, 2 => $b <= $this->contestants ? $b : NULL);
    }
    // This logic changed our zero-based array to one-based so our match
    // ids will line up
    array_unshift($positions, NULL);
    unset($positions[0]);
    $this->data['seeds'] = $positions;
  }

  public function render($theme) {
    // Build our data structure
    $this->build();
    $this->structure('tree');
    return theme($theme, array('structure' => $this->structure));
  }
}