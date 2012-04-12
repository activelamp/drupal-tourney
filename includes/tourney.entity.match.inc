<?php

/**
 * @file
 * Tournament match class.
 */

class TourneyMatch extends Entity {

  private $matchGameIds = array();
  private $matchTournamentId = NULL;
  private $matchFinished = NULL;
  private $matchWinner = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);

    if (property_exists($this, 'id')) {
      $this->fillProperties();
    }
  }

  /**
   * Instantiate a match entity class properly via entity_load().
   *
   * Instantiating the class manually without envoking entity_load() will
   * not allow other modules to hook. Use this static method to create all
   * instances.
   *
   * @return TourneyMatch
   *   A newly instantiated match entity.
   */
   public static function loadTourneyMatch($match_ids = array()) {
     $match_ids = (is_array($match_ids)) ? $match_ids : array();

     $new_match = entity_load('tourney_match', $match_ids);

     return $new_match;
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
   * Sets property values during instantiation.
   *
   * This function _should_ only called by the constructor after our entity 
   * has been saved to the database; relations will exist.
   *
   * @return TourneyMatch
   *   TourneyMatch object for chaining.
   */
  private function fillProperties() {
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
   * Retrieve property. Property created by hook_entity_load().
   */
  public function getWinner() {
    return $this->matchWinner;
  }

  /**
   * Report current win rankings for this match.
   *
   * @see tourney_get_winner_counts()
   */
  public function fetchRanks() {
    $ranks = tourney_get_winner_counts($this);

    return $ranks;
  }

  /**
   * Determine if match is finished (when all games are finished).
   *
   * Do not call this from the constructor, or from any other method that
   * fires during instantiation. If called during instantiation infinite
   * recursion will occur.
   *
   * @return string
   */
  public function isFinished() {
    $games = TourneyGame::loadTourneyGame($this->getGameIds());

    // todo don't like logic that defaults to true.
    $finished = FALSE;
    if (!empty($games)) {
      $finished = TRUE;
      foreach ($games as $game) {
        $finished = ($finished) ? $game->isFinished() : FALSE;
      }
    }
    $this->matchFinished = $finished;

    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->getFinished()) ? 'FINISHED: ' . $this->getWinner() : 'IN PROGRESS...';
      dpr('MATCH ' . $this->id . ': ' . $output);
    }

    return $this->getFinished();
  }

  /**
   * Set the winner property to the winner of this match.
   */
  public function determineWinner() {
    if ($this->isFinished()) {
      $ranks = $this->fetchRanks();
      asort($ranks);
      $keys = array_keys($ranks);
      $this->matchWinner = $keys[0];
    }

    return $this;
  }


}