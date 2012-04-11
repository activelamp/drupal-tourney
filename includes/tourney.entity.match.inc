<?php

/**
 * @file
 * Tournament match class.
 */

class TourneyMatch extends Entity {

  private $finished = FALSE;
  private $gameIds = array();
  private $gameCount = 0;

//  public  $games = array();

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);

    if (property_exists($this, 'id')) {
      $this->fillGameIds()->fillGames()->fillFinished();
    }
  }

  /**
   * Fill property
   */
  private function fillGameIds() {
    $query = relation_query('tourney_match', $this->id);
    $query->entityCondition('bundle', 'has_game');
    $results = $query->execute();

    $game_ids = array();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $game_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
    }

    $this->gameIds = $game_ids;

    return $this;
  }

  /**
   * Fill $this->games array with instantiated TourneyMatch children.
   */
  private function fillGames() {
    //tourney_entity_load()#445 is causing infinite recursion.
    //tourney_game_load_multiple($this->gameIds);

    return $this;
  }

  /**
   * Determine if tournament is finished.
   */
  private function fillFinished() {
    $this->finished = FALSE;

    return $this;
  }
  
  /**
   * Report property.
   */
  public function getGameIds() {
    return $gameIds;
  }
}