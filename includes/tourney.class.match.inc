<?php

/**
 * @file
 * Tournament match class.
 */

class TourneyObject {
  protected $changed = FALSE;

  public function __construct($vars) {
    foreach ( $vars as $key => $value ) {
      $this->$key = $value;
    }
    $this->clearChanged();
  }

  public function __set($name, $value) {
    $this->changed = TRUE;
    $this->$name = $value;
  }

  public function clearChanged() {
    $this->changed = FALSE;
  }

  public function getChanged() {
    return $this->changed;
  }
}

class TourneyMatch extends TourneyObject {
  public $title;
  public $id;
  public $match;
  protected $games = array();
  protected $contestants = array();
  protected $controller;
  protected $winner = NULL;
  protected $loser  = NULL;

  public function __construct($vars) {
    parent::__construct($vars);
    $this->title = 'Match ' . $vars['id'];
    $this->id    = 'match-' . $vars['id'];
    $this->match = $vars['id'];
  }

  public function addContestant($contestant, $slot) {
    if ( !is_object($contestant) ) { 
      if ( !array_key_exists($contestant, $this->controller->data['contestants']) )
        return FALSE;
      $contestant = $this->controller->data['contestants'][$contestant];
    } 
    if ( !array_key_exists($contestant->id, $this->controller->data['contestants']) ) {
      $this->controller->data['contestants'][$contestant->id] = $contestant;
    }
    $this->contestants[$slot] = $contestant->id;
  }

  public function removeContestant($contestant) {
    foreach ( $this->contestants as $slot => $id ) {
      if ( $contestant == $id ) unset($this->contestants[$slot]); 
    }
  }

  // @todo: possibly consolidate all the getSomething and getSomethings together
  public function getContestant($slot) {
    return $this->controller->data['contestants'][$this->contestants[$slot]];
  }

  public function getContestants() {
    $contestants = array();
    foreach ( $this->contestants as $id ) {
      $contestants[$id] = $this->controller->data['contestants'][$id];
    }
    return $contestants;
  }

  public function getContestantIds() {
    return $this->contestants;
  }

  public function addGame($game) {
    $this->games[] = $game;
  }

  public function getGames() {
    $games = array();
    foreach ( $this->games as $id ) {
      $games[$id] = $this->controller->data['games'][$id];
    }
    return $games;
  }

  public function win($contestant) {
    if ( $this->controller->win($this, $contestant) ) {
      $this->winner = $contestant;
      $this->loser  = 
        $this->contestants[$this->contestants[1] == $contestant ? 2 : 1];
    };
  }

  public function determineWinner() {
    // @todo: is this necessary?
  }

  public function getWinner() {
    return $this->controller->data['contestants'][$this->winner];
  }

  public function getLoser() {
    return $this->controller->data['contestants'][$this->loser];
  }

  public function isFinished() {
    // @todo: this. 
  }

  public function isLocked() {
    if ( $this->winner && $this->nextMatch['winner'] ) {

    }
    if ( $this->winner && $this->nextMatch['winner'] ) {

    }
  }

  public function getNextMatch($direction = 'winner') {
    return $this->controller->data['matches'][$this->nextMatch[$direction]];
  }

  public function getPreviousMatches() {
    $matches = array();
    if ( isset($this->previousMatches) ) {
      if ( array_key_exists(0, $this->previousMatches) )
        $matches[] = $this->controller->data['matches'][$this->previousMatches[0]];
      if ( array_key_exists(1, $this->previousMatches) )
        $matches[] = $this->controller->data['matches'][$this->previousMatches[1]];
    }
    return $matches;
  }
}

class TourneyGame extends TourneyObject {
  public $title;
  public $id;
  public $winner;

  public function __construct($vars) {
    parent::__construct($vars);
    $this->title = 'Game ' . $vars['game'];
    $this->id    = 'game-' . $vars['id'];
  }
}