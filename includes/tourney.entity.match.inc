<?php

/**
 * @file
 * Tournament match class.
 */

class TourneyMatch extends Entity {

  private $matchFinished = NULL;
  private $matchGameIds = array();
  private $matchTournamentId = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);

    if (property_exists($this, 'id')) {
      $this->fillGameIds();
    }
  }

  /**
   * Report the game entity_ids related to a match.
   *
   * @param integer $match_id
   *   The match entity id to search for games with.
   *
   * @return array
   *   game entity ids if found, empty array if not found.
   */
  public static function fetchGameIds($match_id) {
    $game_ids = array();

    $query = relation_query('tourney_match', $match_id);
    $query->entityCondition('bundle', 'has_game');
    $results = $query->execute();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $game_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
    }

    return $game_ids;
  }

  /**
   * Fill property
   */
  private function fillGameIds() {
    $this->matchGameIds = TourneyMatch::fetchGameIds($this->id);

    return $this;
  }

  /**
   * Report property.
   */
  public function getGameIds() {
    return $this->matchGameIds;
  }
  
  /**
   * Report property.
   */
  public function getFinished() {
    return $this->matchFinished;
  }

  /**
   * Determine if tournament is finished.
   */
  public function isFinished() {
    $games = TourneyGame::loadTourneyGame($this->getGameIds());

    if (!empty($games)) {
      foreach ($games as $game) {
        dpr($game);
      }
    }
    $this->matchFinished = FALSE;

    return $this->getFinished();
  }

}