<?php
/**
 * @file
 * Tournament series entity class.
 */

class TourneySeriesEntity extends TourneyEntity {

  public $gameIds = array();
  public $contestants = array();

  /**
   * Constructor.
   */
  public function __construct($values = array(), $entityType = 'tourney_series') {
    parent::__construct($values, $entityType);

    if (property_exists($this, 'id') && $this->id) {
      // Add a url to the object
      $this->uri = $this->getUri();
    }
  }

  /**
   * Retrieves a tournament series object for the given tournament.
   *
   * @param $tourney_id
   *   The tournament id to find a series for.
   * @return TourneySeriesEntity
   *   Returns the tourney series object of the tournament.
   */
  public static function getByTournamentId($tourney_id) {
    $query = new EntityFieldQuery();
    $results = $query->entityCondition('entity_type', 'tourney_series')
      ->fieldCondition('tournaments', 'target_id', $tourney_id)
      ->execute();
    if (empty($results)) return false;

    $series = array_shift($results['tourney_series']);
    return tourney_series_load($series->id);
  }
  
  /**
   * Get tournaments in series.
   *
   * @return TourneyTournamentEntity
   *   Returns the loaded tournament objects in series.
   */
  public function getTournaments() {
    $wrapper = entity_metadata_wrapper('tourney_series', $this);
    return $wrapper->tournaments->value();
  }

  public function isFinished() {
    $finished = strlen($this->getWinner());

    return $finished;
  }

  public function isStarted() {
    // TODO: Implement isStarted() method.
  }

  /**
   * Lazy load all the game ids for this series. Once the games have been
   * loaded for the tournament, it will return the 'cached' copy.
   */
  public function getGameIds() {
    if (empty($this->gameIds)) {
      foreach ($this->getTournaments() as $tourney) {
        $this->gameIds = array_merge($this->gameIds, $tourney->getGameIds());
      }

    }
    return $this->gameIds;
  }

  /**
   * Gets a list of participating contestants in seed order from the first round.
   *
   * @return array $contestants
   *   List of contestants, all entities from the first round of a tournament.
   */
  public function getContestants() {
    if (property_exists($this, 'is_new') && $this->is_new) return array();

    foreach ($this->getTournaments() as $tourney) {
      $this->contestants = array_merge($this->contestants, $tourney->getContestants());
    }

    return $this->contestants;
  }

}