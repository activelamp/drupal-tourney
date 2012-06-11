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
      $this->data['rounds'][++$round] = array(
          'title'   => "Round $round",
          'id'      => "round-$round",
      );
      foreach ( range(1, $slots) as $n ) {
        $this->data['matches'][++$match] = array(
          'title'   => "Match $match",
          'id'      => "match-$match",
          'round'   => "round-$round",
        );
      }
    }
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