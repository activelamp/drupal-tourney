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
    // @todo: Remove this hardcoded bundle.
    $this->type = "series";

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

  /**
   * @return array
   *  Returns an array of tournament ids that are on this series.
   */
  public function getTournamentIdArray() {
    $tournaments = $this->getTournaments();
    $tournament_ids = array();
    foreach ($tournaments as $tournament) {
      $tournament_ids[] = $tournament->id;
    }
    return $tournament_ids;
  }

  /**
   * @return string
   *  Returns a comma delimited string of tournaments in this series.
   */
  public function getTournamentsString() {
    $tournaments = $this->getTournamentIdArray();
    $tournament_ids = implode(',', $tournaments);
    return $tournament_ids;
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

class TourneySeriesController {

  /**
   * Constructor
   */
  public function __construct() {
    $this->pluginInfo = self::getPluginInfo(get_class($this));
  }

  public function optionsForm(&$form_state) {
    $form = array();
    $form['controller_description'] = array(
      '#markup' => $this->pluginInfo['description'],
    );

    return $form;
  }

  /**
   * Get the machine name of the plugin.
   *
   * @param $controller_name
   *   The name of the controller to find plugin information for.
   */
  public static function getPluginInfo($controller_name) {
    $plugins = tourney_series_get_types();
    foreach ($plugins as $info) {
      if ($info['controller'] == $controller_name) {
        return $info;
      }
    }
  }
}

