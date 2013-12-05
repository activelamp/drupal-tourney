<?php
use Drupal\esports_tourney\ActiveTourney\ActiveTourneyEntity;
use Drupal\esports_tourney\ActiveTourney\ActiveTourneyHandler;
class LeagueEntity extends Entity {
  public $tourney_ids = NULL;
  public $tourneys = NULL;
  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    // @todo: Remove this hardcoded bundle.
    $this->type = "general_league";
  }

  /**
   * Static utility function that loads
   * all the leagues through entity_load.
   */
  public static function loadAll() {
    if ($leagues = entity_load('tourney_league')) {
      return $leagues;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Static utility function that loads
   * a set of leagues.
   *
   * @param $set
   *    An array of leagues to load.
   */
  public static function loadSet($set) {
    if ($leagues = entity_load('tourney_league', $set)) {
      return $leagues;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Public static function to return all tourney id's for a league.
   *
   * @param $league_id
   *  The id of the league to load tournaments for
   */
  public static function fetchTourneyIds($league_id) {
    // get all tournaments with our league id as a reference.
    $query = db_select('field_data_field_tourney_league_reference', 'r')
      ->fields('r', array('entity_id'))
      ->condition('field_tourney_league_reference_target_id', $league_id)
      ->execute()
      ->fetchAllAssoc('entity_id');

    return array_keys($query);
  }

  /**
   * Public function to fully load all tourneys for a league
   */
  public function getTournaments() {
    if ($this->tourneys == NULL) {
      $this->tourney_ids = LeagueEntity::fetchTourneyIds($this->id);

      // load all tourneys
      $this->tourneys = tourney_tournament_load_multiple($this->tourney_ids);
    }
    return $this->tourneys;
  }

  /**
   * Public function to get all teams in a league
   */
  public function getTeams($tourney_id = FALSE) {
    $tourneys = $this->getTournaments();

    if ($tourney_id && isset($tourneys[$tourney_id])) {
      return $tourneys[$tourney_id]->getContestants();
    }
    else {
      $return = array();
      foreach($tourneys as $tourney_id => $tourney) {
        $return[] = array("tourney_id" => $tourney_id) 
          + $tourney->getContestants();
      }
      return $return;
    }
  }

  /**
   * public function to render a tournament
   */
  public function getStandingsView($tourney = FALSE) {
    if (!$tourney) {
      $tourneys = !$this->tourneys ? $this->getTournaments() : $this->tourneys;
      
      $return = array();
      foreach ($tourneys as $tourney_id => $tourney) {
        $instance = ActiveTourneyHandler::getInstance($tourney);
        $return[$tourney_id]["standings"] = $instance->getStandingsView();
      }
      return $return;
    }
    else {
      if (is_int($tourney)) {
        $tourney = ActiveTourneyEntity::getInstance()->getEntity($tourney);
      }      
      $instance = ActiveTourneyHandler::getInstance($tourney);
      return $instance->getStandingsView();
    }
  }

  /**
   * Public function to get the schedule of a tourney
   */
  public function getLeagueSchedule() {
    $matches = array();
    $games = array();
    $schedule = array();

    // get all tourneys
    $tourneys = !$this->tourneys ? $this->getTournaments() : $this->tourneys;

    // get all matches for all tourneys, then all games, then load first game in each set.    
    foreach($tourneys as $tourney_id => $tourney) {
      $matches += $tourney->getMatchIds();
    }
    $loaded_matches = tourney_match_load_multiple($matches);
    foreach($loaded_matches as $match_id => $match) {
      $games[] = reset($match->gameIds);
    }
    $loaded_games = tourney_game_load_multiple($games);
    foreach($loaded_games as $game_id => $game) {
      $schedule[$game_id] = $game->field_game_date[LANGUAGE_NONE][0]['value'];      
    }
    // flip the array to preserve keys, sort, flip again
    $temp = array_flip($schedule);
    ksort($temp);
    $schdule = array_flip($temp);
    return $schedule;
  }
}
