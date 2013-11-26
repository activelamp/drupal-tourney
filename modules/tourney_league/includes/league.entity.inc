<?php

class LeagueEntity extends Entity {
  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    // @todo: Remove this hardcoded bundle.
    $this->type = "lol";
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
    $query = db_query('field_data_field_tourney_league_reference', 'r')
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
    $tourney_ids = LeagueEntity::fetchTourneyIds($this->id);

    // load all tourneys
    $tourneys = tourney_tournament_load($tourney_ids);

    return $tourneys;
  }
}
