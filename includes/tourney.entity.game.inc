<?php

/**
 * @file
 * Tournament game class.
 *
 * @todo
 * Error checking and naming convention.
 */

class TourneyGame extends Entity {

  // I can't help myself. Must... line... up... Resistance... Futile...
  private $_entitySaved  = FALSE;
  private $_finished     = FALSE;
  private $_tournamentId = NULL;
  private $_matchId      = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);

    // Game entity may only exist in memory if created during our runtime.
    if (property_exists($this, 'id')) {
      // Game must be saved before a relationship will exist.
      if ($this->_fillEntitySaved() == TRUE) {
        $this->_fillMatchId()->_fillTournamentId()->_fillFinished();
      }
    }
  }

  /**
   * Report the match entity_id related to a game.
   *
   * @return integer
   *   match entity id if found, 0 if no associated match exists.
   */
  public static function getMatchId($game_id) {
    $match_id = 0;
    
    $query = relation_query('tourney_game', $game_id);
    $query->entityCondition('bundle', 'has_game');
    $results = $query->execute();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $match_id = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
    }

    return $match_id;
  }
  
  /**
   * Report the tournament entity_id related to a game.
   *
   * @param integer $game_id
   * @param integer $match_id
   *   (optional) If provided will eliminate some work.
   *
   * @return integer $tournament_id
   */
  public static function getTournamentId($game_id, $match_id = NULL) {
    $tournament_id = 0;
    $match_id = ($match_id) ? $match_id : TourneyGame::getMatchId($game_id);

    if ($match_id) {
      $query = relation_query('tourney_match', $match_id);
      $query->entityCondition('bundle', 'has_match');
      $results = $query->execute();
      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        $tournament_id = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
      }
    }

    return $tournament_id;    
  }

  /**
   * Fill property.
   * @todo Change code to waste less. Why retrieve match_id and throw it away?
   */
  private function _fillEntitySaved() {
    $this->_entitySaved = (TourneyGame::getMatchId($this->id)) ? TRUE : FALSE;

    // No chaining!
    return $this->_entitySaved;
  }

  /**
   * Fill property.
   */
  private function _fillMatchId() {
    $this->_matchId = TourneyGame::getMatchId($this->id);

    return $this;
  }
  
  /**
   * Fill property.
   */
  private function _fillTournamentId() {
    // Passing a valid match_id quickens response.
    $this->_tournamentId = TourneyGame::getTournamentId($this->id, $this->_matchId);

    return $this;
  }
  
  /**
   * Fill property.
   */
  private function _fillFinished() {
    $this->_finished = strlen($this->winner) ? TRUE : FALSE;

    return $this;
  }
}