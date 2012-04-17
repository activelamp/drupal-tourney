<?php

/**
 * @file
 * Tournament match class.
 */

class TourneyMatch extends Entity {

  private $matchGameIds = NULL;
  private $matchTournamentId = NULL;
  private $matchStarted = NULL;
  private $matchFinished = NULL;
  private $matchWinner = NULL;
  private $matchSetup = NULL;
  
  public $matchGames = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    $this->setup();
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
  public static function fetchGameId($match_id) {
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
   * Report the tournament entity_id related to a match.
   *
   * @param integer|array $match_ids
   *   The match entity id to search for games with.
   *
   * @return array $tournament_ids
   *   game entity ids if found, empty array if not found.
   */
  public static function fetchTournamentId($match_ids, &$tournament_ids = array()) {
    if (is_array($match_ids)) {
      foreach($match_ids as $match_id) {
        $tournament_ids = TourneyMatch::fetchTournamentIds($match_id, $tournament_ids);
      }
    }
    else {
      // todo : Integrity check.
      //if (!is_integer($match_ids)) {
      //  throw new Exception(t('Required parameter is invalid: ') . $match_ids);
      //}
    }
    $match_id = $match_ids;

    $query = relation_query('tourney_match', $match_id);
    $query->entityCondition('bundle', 'has_match');
    $results = $query->execute();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $tournament_ids[$match_id] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
    }

    return $tournament_ids;
  }

  /**
   * Sets property values during instantiation.
   *
   * Do not call this from the constructor, or from any other method that
   * fires during instantiation. If called during instantiation infinite
   * recursion may occur.
   * @see TourneyMatch::isStarted()
   * @see TourneyMatch::isFinished()
   *
   * @return TourneyMatch
   *   TourneyMatch object for chaining.
   */
  public function setup() {
    if (!$this->getSetup()) {
      $this->matchSetup = TRUE;
      $this->matchGameIds = TourneyMatch::fetchGameId($this->id);
      $this->matchTournamentId = TourneyMatch::fetchTournamentId($this->id);
      $this->matchStarted = $this->isStarted();
      $this->matchFinished = $this->isFinished();
      $this->matchWinner = $this->determineWinner();
      
      $this->matchGames = TourneyGame::loadTourneyGame($this->getGameIds());
    }

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
   * Retrieve property.
   */
  public function getWinner() {
    return $this->matchWinner;
  }

  /**
   * Retrieve property
   */
  public function getStarted() {
    return $this->matchStarted;
  }

  /**
   * Retrieve property
   */
  public function getSetup() {
    return $this->matchSetup;
  }

  /**
   * Report current winner rankings for this match.
   *
   * @return array
   *   Array is keyed by winner and sorted by descending value.
   *
   * @see tourney_get_winner_counts()
   */
  public function fetchRanks() {
    $ranks = tourney_get_winner_counts($this);
    arsort($ranks);

    return $ranks;
  }

  /**
   * Determine if match is started (when any relationship is defined).
   *
   * Do not call this from the constructor, or from any other method that
   * fires during instantiation. If called during instantiation infinite
   * recursion may occur.
   *
   * @return bool
   */
  public function isStarted() {
    if (!$this->getSetup()) {
      throw new Exception(t('Match entity must first be setup before use.'));
    }

    $games = TourneyGame::loadTourneyGame($this->getGameIds());
    $started = FALSE;

    if (!empty($games)) {
      foreach ($games as $game) {
        $started = ($started) ? TRUE : $game->isStarted();
      }
    }

    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->getStarted()) ? 'STARTED.' : 'NOT STARTED...';
      dpr('MATCH ' . $this->id . ': ' . $output);
    }

    return $started;
  }

  /**
   * Determine if match is finished (when all games are finished).
   *
   * Do not call this from the constructor, or from any other method that
   * fires during instantiation. If called during instantiation infinite
   * recursion may occur.
   *
   * @return bool
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

    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->getFinished()) ? 'FINISHED: ' . $this->getWinner() : 'IN PROGRESS...';
      dpr('MATCH ' . $this->id . ': ' . $output);
    }

    return $finished;
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