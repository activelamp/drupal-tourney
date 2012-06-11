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

      foreach ( range(1, $slots) as $n ) {
        $this->data['matches'][++$match] = 
          $this->buildMatch(array(
            'id' => $match,
            'round' => "round-$round",
          ));
      }
    }
  }

  public function buildRound($data) {
    $round = array(
      'title' => 'Round ' . $data['id'],
      'id'    => 'round-' . $data['id'],
    );
    unset($data['id']);
    //$round += $data;
    return $round;
  }

  public function buildMatch($data) {
    $match = array(
      'title'   => "Match " . $data['id'],
      'id'      => "match-" . $data['id'],
    );
    unset($data['id']);
    $match += $data;
    return $match;
  }

  public function structure() {
    $structre = array();
    foreach ( $this->data['rounds'] as $round ) {
      $structure[$round['id']] = $round + array('matches' => array());
    }
    foreach ( $this->data['matches'] as $match ) {
      $structure[$match['round']]['matches'][$match['id']] = $match;
    }
    $this->structure = $structure;
  }

  public function determineWinner($tournament) {

  }

  public function isFinished($tournament) {

  }
}