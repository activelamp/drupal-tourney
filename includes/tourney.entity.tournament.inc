<?php

/**
 * @file
 * Tournament class.
 */

class TourneyTournament extends Entity {

  private $tournamentMatchIds = array();
  private $tournamentFinished = NULL;
  private $tournamentWinner   = NULL;
  private $tournamentPlugin   = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    if (property_exists($this, 'id')) {
      $this->fillProperties();
    }
  }

  /**
   * STATIC
   */
  public static function fetchMatchIds($tournament_id) {
    $query = relation_query('tourney_tournament', $tournament_id);
    $query->entityCondition('bundle', 'has_match');
    $results = $query->execute();

    $match_ids = array();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $match_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
    }

    return $match_ids;
  }

  /**
   * Reports if a tournament exists or not.
   *
   * The actual content of the passed parameter should throw no errors. If
   * there is anything wrong the reported value will be FALSE.
   *
   * @todo Logic for direct database query.
   *
   * @param integer $tournament_id
   *
   * @return bool $exists
   *   Will return TRUE if $tournament_id exists in the database, FALSE if
   *   the parameter can not be confirmed as a tournament_id.
   */
  public static function exists($tournament_id = NULL) {
    $exists = FALSE;

    $tournament_id = is_string($tournament_id) ? intval($tournament_id) : 0;    
    if (is_integer($tournament_id) && $tournament_id > 0) {
      // TODO : Direct database query.
      $query = TRUE;
      if ($query) {
        $exists = TRUE;
      }
    }

    return $exists;
  }

  /**
   * Sets property values during instantiation.
   *
   * This function _should_ only called by the constructor after our entity 
   * has been saved to the database; relations will exist.
   *
   * @return TourneyTournament
   *   TourneyTournament object for chaining.
   */
  private function fillProperties() {
    if (!class_exists($this->format)) {
      throw new Exception(t('Plugin does not exist'));
    }

    $this->tournamentMatchIds = TourneyTournament::fetchMatchIds($this->id);
    $this->tournamentPlugin = new $this->format(0);

    return $this;
  }

  /**
   * Report property.
   */
  public function getMatchIds() {
    return $this->tournamentMatchIds;
  }

  /**
   * Report property.
   */
  public function getFinished() {
    return $this->tournamentFinished;
  }

  /**
   * Report current win rankings for this match.
   *
   * @see tourney_get_winner_counts()
   */
  public function fetchRanks() {
    $ranks = array();
    $matches = TourneyMatch::loadTourneyMatch($this->getMatchIds());

    if (!empty($matches)) {
      foreach ($matches as $match) {
        $ranks[] = $match->fetchRanks();
      }
    }

    return $ranks;
  }

  /**
   * Determine if a tournament is finished (when all matches are finished).
   *
   * Do not call this from the constructor, or from any other method that
   * fires during instantiation. If called during instantiation infinite
   * recursion will occur.
   *
   * @return TourneyTournament $this
   *   $this
   */
  public function isFinished() {
    $finished = $this->tournamentPlugin->is_finished($this);
    $this->tournamentFinished = $finished;

    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->getFinished()) ? 'FINISHED!' : 'IN PROGRESS...';
      dpr('TOURNAMENT ' . $this->id . ': ' . $output);
    }

    return $this->getFinished();
  }

  /**
   * Set the winner property to the winner of this match.
   */
  public function determineWinner() {
    if ($this->isFinished()) {
      $this->tournamentPlugin->determine_winner($this);
    }

    return $this;
  }

}