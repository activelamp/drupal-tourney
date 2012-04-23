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
  
  private $matchGames = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
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
   * Report the tournament entity_id related to a match.
   *
   * @param integer|array $match_ids
   *   The match entity id to search for games with.
   *
   * @return array $tournament_ids
   *   tournament entity id if found, empty array if not found.
   */
  public static function fetchTournamentId($match_ids, &$tournament_ids = array()) {
    if (is_array($match_ids)) {
      foreach($match_ids as $match_id) {
        $tournament_ids = TourneyMatch::fetchTournamentIds($match_id, $tournament_ids);
      }
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
   * Retrieve property. (Lazy load TourneyGame entities).
   *
   * @example
   *   $games47_50 = $match->getGames();
   *   $game47     = $match->getGames('game.47');
   *   $game47     = $match->getGames(0);
   *   $game48     = $match->getGames(1);
   *   $game48_id  = $match->getGames(1)->id;
   *
   * @param string|integer|NULL $key
   *   (optional) Associated key index, or simple integer offset index.
   *
   * @return array|TourneyGame $game
   *   If the $key paramater was NULL then an array of all games keyed by
   *   ['game'.$game_id]. If $key parameter was string then only the
   *   element [$key] of all games is returned. If $key parameter is an
   *   integer then only the element at offset [$key] is returned.
   */
  function getGames($key = NULL) {
    if (is_null($this->matchGames)) {
      foreach($this->getMatchIds() as $matchId)
      $this->matchGames['match.' . $matchId] = tourney_match_load($matchId);
    }

    // When the parameter is of the type integer then we will just report
    // the element contained in the array at the integer offset.
    if (is_integer($key)) {
      $keys = array_keys($this->matchGames);
      $key = $keys[$key];
    }

    // If a key was specified only report that array element, otherwise
    // report the entire array.
    $game = ($key) ? $this->matchGames[$key] : $this->matchGames;

    return $game;
  } 

  /**
   * Report property.
   *
   */
  public function getTournamentId() {
    if (is_null($this->matchTournamentId)) {
      $this->matchTournamentId = TourneyMatch::fetchTournamentId($this->id);
    }

    return $this->matchTournamentId;
  }

  /**
   * Report property.
   */
  public function getGameIds() {
    if (is_null($this->matchGameIds)) {
      $this->matchGameIds = TourneyMatch::fetchGAmeIds($this->id);
    }

    return $this->matchGameIds;
  }

  /**
   * Retrieve property.
   */
  public function getWinner() {
    if (is_null($this->matchWinner)) {
      $this->matchWinner = $this->determineWinner();
    }

    return $this->matchWinner;
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
   * @return bool
   */
  public function isStarted() {
    if (!$this->id) {
      throw new Exception(t('Match entity must first be setup before use.'));
    }

    $games = tourney_game_load_multiple($this->getGameIds());
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
    $games = tourney_game_load_multiple($this->getGameIds());

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