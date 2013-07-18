<?php
/**
 * @file
 * Tournament series entity class.
 */

class TourneySeriesEntity extends TourneyEntity {

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
  public static function getByTournament($tourney_id) {

  }
  
  /**
   * Get tournaments in series.
   */
  public function getTournaments() {
    // TODO: Implement isFinished() method.
  }

  public function isFinished() {
    // TODO: Implement isFinished() method.
  }

  public function isStarted() {
    // TODO: Implement isStarted() method.
  }

}