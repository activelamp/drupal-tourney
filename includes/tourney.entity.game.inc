<?php

/**
 * @file
 * Tournament game class.
 *
 */

class TourneyGame extends TourneyEntity {

  public  $tournamentId = NULL;
  public  $matchId = NULL;
  private $gameContestantIds = NULL;
  private $gameContestants = NULL;
  private $gameStarted = NULL;
  private $gameFinished = NULL;
  private $gameWinner = NULL;

  /**
   * Constructor.
   */
  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    
    $this->tournamentId = $this->getTournamentId($this->id);
    $this->matchId = $this->getMatchId($this->id);
  }

  /**
   * Report the match entity_id related to a game.
   *
   * @return array $match_ids
   *  An integer or associative array, keyed by TourneyGame id, of TourneyMatch
   *  ids. Array will be empty if no matches are found.
   */
  public function getMatchId($game_id) {
    if (is_null($this->matchId)) {
      $game_ids  = (is_array($game_id)) ? $game_id : array($game_id);
      $match_ids = array();

      $query = relation_query('tourney_game', $game_ids);
      $query->entityCondition('bundle', 'has_game');
      $results = $query->execute();
      if (!empty($results)) {
        foreach ($results as $relation) {
          $r = relation_load($relation->rid);
          $this->matchId = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
          break;
        }
      }
    }
    return $this->matchId;
  }

  /**
   * Get the tournament entity_id related to a game.
   *
   * @param integer $game_id
   * @param integer $match_id
   *   (optional) If provided will eliminate some work.
   *
   * @return integer|NULL $tournament_id
   */
  public function getTournamentId($game_id, $match_id = NULL) {
    if (is_null($this->tournamentId)) {
      $tournament_id = NULL;
      $match_id = ($match_id) ? array($game_id => $match_id) : $this->getMatchIds($game_id);

      $query = relation_query('tourney_match', $match_id[$game_id]);
      $query->entityCondition('bundle', 'has_match');
      $results = $query->execute();
      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        $this->tournamentId = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
        break;
      }
    }
    return $this->tournamentId;
  }

  /**
   * Report the contestants related to a game.
   *
   * @param integer $game_id
   * @param integer $match_id
   *   (optional) If provided will eliminate some work.
   *
   * @return array $contestants
   *   An associative array containing:
   *   - contestant: An array of entity type concatenated with entity id:
   *     - type: String denoting the entity type.
   *     - id: Integer denoting the specific entity id.
   *   An empty array will be returned if no contestants are found.
   */
  public static function fetchContestantIds($game_id, $match_id = NULL) {
    $contestants = array();
    $match_id = ($match_id) ? array($game_id => $match_id) : $this->getMatchIds($game_id);

    if ($match_id) {
      $query = relation_query('tourney_match', $match_id[$game_id]);
      $query->entityCondition('bundle', array(TOURNEY_CONTESTANT_1, TOURNEY_CONTESTANT_2), 'IN');
      $results = $query->execute();
      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        $type = $r->endpoints[LANGUAGE_NONE][1]['entity_type'];
        $id = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
        $contestants[$type . '.' . $id] = array(
          'type' => $type,
          'id' => $id,
        );
      }
    }
    
    return $contestants;
  }

  /**
   * Retrieve property. (Lazy load contestant objects).
   *
   * Example:
   * @code
   *   $contestant10_11 = $game->getContestants();
   *   $contestant10    = $game->getContestants('user.10');
   *   $contestant10    = $game->getContestants(0);
   *   $contestant11    = $game->getContestants(1);
   *   $contestant11_id = $game->getContestants(0)->id;
   * @endcode
   *
   * @param string|integer|NULL $key
   *   (optional) Associated key index, or simple integer offset index.
   *
   * @return array|object $contestant
   *   If the $key paramater was NULL then an array of all contestants keyed 
   *   by ["$obj_type.$obj_id"]. If $key parameter was string then only the
   *   element [$key] of all contestants is returned. If $key parameter is an
   *   integer then only the element at offset [$key] is returned.
   */
  public function getContestants($key = NULL) {
    if (is_null($this->gameContestants)) {
      foreach ($this->getContestantIds() as $c_id) {
        $this->gameContestants[$c_id['type'] . '.' . $c_id['id']] = entity_load($c_id['type'], array($c_id['id']));
      }
    }

    // When the parameter is of the type integer then we will just report
    // the element contained in the array at the integer offset.
    if (is_integer($key)) {
      $keys = array_keys($this->gameContestants);
      $key = $keys[$key];
    }

    // If a key was specified only report that array element, otherwise
    // report the entire array.
    $contestant = ($key) ? $this->gameContestants[$key] : $this->gameContestants;

    return $contestant;
  }

  /**
   * Retrieve property. Pseudo caches result.
   */
  public function getMatchId() {
    if (is_null($this->matchId)) {
      $match_ids = $this->getMatchIds($this->id);
      $this->matchId = $match_ids[$this->id];
    }

    return $this->matchId;
  }

  /**
   * Retrieve property. Pseudo caches result.
   */
  public function getContestantIds() {
    if (is_null($this->gameContestantIds)) {
      $this->gameContestantIds = TourneyGame::fetchContestantIds($this->id, $this->getMatchId());
    }

    return $this->gameContestantIds;
  }

  /**
   * Determine if a game has started (contestant relationships exist).
   *
   * @return bool
   */
  public function isStarted() {
    $started = !empty($this->getContestantId) ? TRUE : FALSE;

    return $started;
  }

  /**
   * Determine if a game is finshed (if a winner has been set).
   *
   * @return bool
   */
  public function isFinished() {
    $finished = strlen($this->getWinner());

    return $finished;
  }
}