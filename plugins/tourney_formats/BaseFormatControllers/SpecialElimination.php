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

  public function __construct($contestants) {
    $this->contestants = $contestants;  
    $this->slots = pow(2, ceil(log($contestants, 2)));
    $this->build();
    $this->structure();
  }

  public function build() {
    $match = 0;
    $slots = $this->slots;
    $round = 0;
    while ( ($slots /= 2) >= 1 ) {
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round));

      foreach ( range(1, $slots) as $roundMatch ) {
        $this->data['matches'][++$match] = 
          $this->buildMatch(array(
            'id' => $match,
            'round' => $round,
            'roundMatch' => (int) $roundMatch,
          ));
      }
    }
    $this->populatePositions();
  }

  public function buildRound($data) {
    $round = array(
      'title' => 'Round ' . $data['id'],
      'id'    => 'round-' . $data['id'],
    );
    unset($data['id']);
    $round += $data;
    return $round;
  }

  public function buildMatch($data) {
    $match = array(
      'title'   => "Match " . $data['id'],
      'id'      => "match-" . $data['id'],
      'match'   => $data['id'],
    ) + $data;
    return $match;
  }

  public function populatePositions() {
    foreach ( $this->data['matches'] as &$match ) {
      // Next match properties
      $next = $this->find('matches', array(
        'round' => $match['round'] + 1,
        'roundMatch' => (int) ceil($match['roundMatch'] / 2),
      ), 'id');
      if ( $next ) $match['nextMatch'] = array_pop($next);

      // Previous match properties
      $target = $match['roundMatch'] * 2;
      $prev = $this->find('matches', array(
        'round' => $match['round'] - 1,
        'roundMatch' => array($target, $target - 1),
      ), 'id');
      if ( $prev ) {
        $match['previousMatches'] = $prev;
      }
    }
  }

  public function find($data, $vars, $specific = NULL) {
    if ( !array_key_exists($data, $this->data) ) return NULL;
    $elements = array();
    // is_array is expensive, set up an array to store this information
    $is_array = array();
    foreach ( $vars as $key => $value )
      $is_array[$key] = is_array($value);
    foreach ( $this->data[$data] as $id => $element ) {
      foreach ( $vars as $key => $value ) {
        if ( $element[$key] !== $value ) {
          if ( !$is_array[$key] || !in_array($element[$key], $value) ) continue 2;
        }
      }
      if ( $specific !== NULL )
        $elements[] = $element[$specific];
      else 
        $elements[] = $element;
    }
    return $elements;
  }

  public function calculateNextPosition($match) {
    $round = $match['round'] + 1;
    return ( $round + ceil($match['roundMatch']/2) );
  }

  public function structure() {
    $structre = array();
    foreach ( $this->data['rounds'] as $round ) {
      $structure[$round['id']] = $round + array('matches' => array());
    }
    foreach ( $this->data['matches'] as $match ) {
      $structure['round-' . $match['round']]['matches'][$match['id']] = $match;
    }
    $this->structure = $structure;
  }

  public function determineWinner($tournament) {

  }

  public function isFinished($tournament) {

  }
}