<?php

/**
 * @file
 * Tournament class.
 */

class TourneyTournament extends Entity {

  private $tournamentMatchIds = array();
  private $tournamentStarted  = NULL;
  private $tournamentFinished = NULL;
  private $tournamentWinner = NULL;
  private $tournamentPlugin = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    $this->setup();
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
   * @return TourneyTournament
   *   TourneyTournament object for chaining.
   */
  private function setup() {
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
   * Report current winner rankings for this tournament.
   *
   * @return array
   *   associative array.
   *
   * @see TourneyMatch::fetchRanks()
   */
  public function fetchRanks() {
    $ranks = array();
    $ranks['game_wins'] = array();
    $ranks['match_wins'] = array();
    $ranks['match_loss'] = array();
    $ranks['contestants'] = array();

    $matches = TourneyMatch::loadTourneyMatch($this->getMatchIds());

    if (!empty($matches)) {
      foreach ($matches as $match) {
        // Compile array of game wins keyed by [match_id][contestant].
        $ranks['game_wins'][$match->id] = $match->fetchRanks();
        // Compile array of all contestants in this tournament.
        $ranks['contestants'] = array_merge($ranks['contestants'], $ranks['game_wins'][$match->id]);
      }
      $ranks['contestants'] = array_keys($ranks['contestants']);
    }
    
    if (!empty($matches)) {
      foreach ($ranks['contestants'] as $contestant) {
        // Initialize all array keys to 0. Avoiding php notices further down.
        $ranks['match_wins'][$contestant] = 0;
        $ranks['match_loss'][$contestant] = 0;
      }
      $ranks['match_wins']['NA'] = 0;
      $ranks['match_loss']['NA'] = 0;
      foreach ($matches as $match) {
        // Sum total number of wins and losses keyed by contestant.
        $keys = array_keys($ranks['game_wins'][$match->id]);
        $keys = (empty($keys)) ? array('NA', 'NA') : $keys;
        $ranks['match_wins'][$keys[0]] += 1;
        $ranks['match_loss'][$keys[1]] += 1;
      }
      // Sort the arrays by descending values.
      arsort($ranks['match_wins']);
      arsort($ranks['match_loss']);
    }

    return $ranks;
  }

  /**
   * Determine if a tournament is finished (when all matches are finished).
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