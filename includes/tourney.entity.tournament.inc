<?php

/**
 * @file
 * Tournament class.
 */

class TourneyTournament extends Entity {

  private $matchIds = array();
  private $finished = FALSE;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);

    if (property_exists($this, 'id')) {
      $this->fillMatchIds()->fillFinished();
    }
  }

  /**
   * Fill this->$matchIds array with all entity_ids for associated matches.
   *
   * @return
   *   $this for chaining.
   */
  private function fillMatchIds() {
    $query = relation_query('tourney_tournament', $this->id);
    $query->entityCondition('bundle', 'has_match');
    $results = $query->execute();

    $match_ids = array();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $match_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
    }
    
    $this->matchIds = $match_ids;
    
    return $this;
  }

  /**
   * Determine if tournament is finished.
   */
  private function fillFinished() {
    $this->finished = FALSE;
    
    return $this;
  }
}