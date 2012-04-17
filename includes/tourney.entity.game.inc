<?php

/**
 * @file
 * Tournament game class.
 *
 * Check the status of $this->entitySaved before doing anything important.
 * It is possible a method may be called while the entity object still does
 * not have an id assigned or does not have any relationships defined. Our
 * object will not have an id if it was just created during the current
 * runtime. Our object will not fill (assign values during construction)
 * all properties unless it has been previously saved to the database at
 * least once. 
 *
 * @todo
 * Error checking and naming convention.
 */

class TourneyGame extends Entity {

  private $gameSaved  = NULL;
  private $tournamentId = NULL;
  private $matchId      = NULL;
  private $gameFinished = NULL;
  private $gameStarted  = NULL;

  /**
   * Constructor.
   */
  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);

    // Game entity may only exist in memory if created during our runtime.
    if (property_exists($this, 'id')) {
      // Game must be saved before a relationship will exist.
      if ($this->fillGameSaved() == TRUE) {
        $this->fillProperties();
      }
    }
  }

  /**
   * Instantiate a game entity class properly via entity_load().
   *
   * Instantiating the class manually without envoking entity_load() will
   * not allow other modules to hook. Use this static method to create all
   * instances.
   *
   * @return TourneyGame
   *   A newly instantiated game entity.
   */
   public static function loadTourneyGame($game_ids = array()) {
     $game_ids = (is_array($game_ids)) ? $game_ids : array();

     $new_game = entity_load('tourney_game', $game_ids);

     return $new_game;
   }

  /**
   * Report the match entity_id related to a game.
   *
   * @return integer
   *   match entity id if found, 0 if no associated match exists.
   */
  public static function fetchMatchId($game_id) {
    $match_id = 0;

    $query = relation_query('tourney_game', $game_id);
    $query->entityCondition('bundle', 'has_game');
    $results = $query->execute();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $match_id = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
    }

    return $match_id;
  }

  /**
   * Report the tournament entity_id related to a game.
   *
   * @param integer $game_id
   * @param integer $match_id
   *   (optional) If provided will eliminate some work.
   *
   * @return integer $tournament_id
   */
  public static function fetchTournamentId($game_id, $match_id = NULL) {
    $tournament_id = 0;
    $match_id = ($match_id) ? $match_id : TourneyGame::getMatchId($game_id);

    if ($match_id) {
      $query = relation_query('tourney_match', $match_id);
      $query->entityCondition('bundle', 'has_match');
      $results = $query->execute();
      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        $tournament_id = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
      }
    }

    return $tournament_id;
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
   */
  public static function fetchContestantIds($game_id, $match_id = NULL) {
    $contestants = array();
    $match_id = ($match_id) ? $match_id : TourneyGame::fetchMatchId($game_id);

    if ($match_id) {
      $query = relation_query('tourney_match', $match_id);
      $query->entityCondition('bundle', TOURNEY_CONTESTANT_1);
      $results = $query->execute();
      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        $contestants[] = array(
          $r->endpoints[LANGUAGE_NONE][1]['entity_type'],
          $r->endpoints[LANGUAGE_NONE][1]['entity_id'],
        );
      }
    }

    return $contestants;
  }

  /**
   * Fill property during object load. Entity is expected to have been
   * previously saved. All fill methods are similar and may normally be
   * chained.
   *
   * Uses TourneyGame::getMatchID() to fill.
   *
   * @todo Why retrieve match_id and throw it away?
   *
   * @return bool
   *   TRUE if the game has been saved subsequent to creation, FALSE if it
   *   exists in memory only. Do not chain this method.
   */
  private function fillGameSaved() {
    $this->gameSaved = (TourneyGame::fetchMatchId($this->id)) ? TRUE : FALSE;

    // No chaining!
    return $this->getSaved();
  }

  /**
   * Sets property values during instantiation.
   *
   * This function is only called by the constructor after our entity has
   * been saved to the database; relations will exist.
   *
   * @return TourneyGame
   *   TourneyGame object for chaining.
   */
  private function fillProperties() {
    // Retrieve associated match_id.
    $this->matchId = TourneyGame::fetchMatchId($this->id);
    // Passing an existing match_id quickens response.
    $this->tournamentId = TourneyGame::fetchTournamentId($this->id, $this->matchId);
    $this->gameStarted  = $this->isStarted();
    $this->gameFinished = $this->isFinished();

    return $this;
  }

  /**
   * Retrieve property.
   */
  public function getSaved() {
    return $this->gameSaved;
  }

  /**
   * Retrieve property.
   */
  public function getFinished() {
    return $this->gameFinished;
  }

  /**
   * Retrieve property.
   */
  public function getStarted() {
    return $this->gameStarted;
  }

  /**
   * Retrieve property. Property created by hook_entity_load().
   */
  public function getWinner() {
    return $this->winner;
  }

  /**
   * Determine if a game has started (contestant relations have been setup).
   *
   * Records result to class property.
   */
  public function isStarted() {
    $contestants = TourneyGame::fetchContestantIds($this->id);

    $this->gameStarted = empty($contestants) ? FALSE : TRUE;

    return $this->getStarted();
  }

  /**
   * Determine if a game is finshed (when $this->winner has value).
   *
   * Records result to class property.
   */
  public function isFinished() {
    $this->gameFinished = strlen($this->getWinner()) ? TRUE : FALSE;

    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->getFinished()) ? 'FINISHED: ' . $this->getWinner() : 'IN PROGRESS...';
      dpr('GAME ' . $this->id . ': ' . $output);
    }

    return $this->getFinished();
  }
}