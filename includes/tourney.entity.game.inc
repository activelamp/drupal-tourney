<?php

/**
 * @file
 * Tournament game class.
 *
 * @todo
 * Error checking and naming convention.
 */

class TourneyGame extends Entity {

  private $gameSetup = NULL;
  private $gameStarted = NULL;
  private $gameFinished = NULL;
  private $gameTournamentId = NULL;
  private $gameContestantIds = NULL;
  private $gameMatchId = NULL;
  private $gameWinner = NULL;
  
  public $gameContestants = NULL;

  /**
   * Constructor.
   */
  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    $this->setup();
  }

  /**
   * Instantiate a game entity class properly via entity_load().
   *
   * Instantiating the class manually without envoking entity_load() will
   * not allow other modules to hook. Use this static method to create all
   * instances.
   *
   * @return TourneyGame
   *   A array of newly instantiated game entities.
   */
   public static function loadTourneyGame($game_ids = array()) {
     $game_ids = (is_array($game_ids)) ? $game_ids : array($game_ids);

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
    $match_id = NULL;

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
    $match_id = ($match_id) ? $match_id : TourneyGame::fetchMatchId($game_id);

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
   *     - object: Contestant object.
   */
  public static function fetchContestantIds($game_id, $match_id = NULL) {
    $contestants = array();
    $match_id = ($match_id) ? $match_id : TourneyGame::fetchMatchId($game_id);

    if ($match_id) {
      $query = relation_query('tourney_match', $match_id);
      $query->entityCondition('bundle', array(TOURNEY_CONTESTANT_1, TOURNEY_CONTESTANT_2), 'IN');
      $results = $query->execute();
      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        $type = $r->endpoints[LANGUAGE_NONE][1]['entity_type'];
        $id = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
        $contestants[$type .'.'. $id] = array(
          'type' => $type,
          'id' => $id,
        );
      }
    }
    
    return $contestants;
  }

  /**
   * Fetches and assigns property values.
   *
   * @return TourneyGame
   *   TourneyGame object for chaining.
   */
  public function setup() {
    // No setup for newly created entities that haven't been saved yet.
    if (empty($this->id)) {
      return;
    }  
    $this->gameMatchId = TourneyGame::fetchMatchId($this->id);
    $this->gameTournamentId = TourneyGame::fetchTournamentId($this->id, $this->getMatchId());
    $this->gameContestantIds = TourneyGame::fetchContestantIds($this->id, $this->getMatchId());
    $this->gameStarted  = $this->isStarted();
    $this->gameFinished = $this->isFinished();
    $this->gameWinner = $this->winner;
    
    foreach($this->getContestantIds() as $c_id) {
      $this->gameContestants[] = entity_load($c_id['type'], array($c_id['id']));
    }

    return $this;
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
  public function getMatchId() {
    return $this->gameMatchId;
  }

  /**
   * Retrieve property.
   */
  public function getStarted() {
    return $this->gameStarted;
  }

  /**
   * Retrieve property.
   */
  public function getContestantIds() {
    return $this->gameContestantIds;
  }

  /**
   * Retrieve property. Property created by hook_entity_load().
   */
  public function getWinner() {
    return $this->gameWinner;
  }

  /**
   * Determine if a game has started (contestant relationships exist).
   */
  public function isStarted() {
    $started = !empty($this->getContestantId) ? TRUE : FALSE;
    
    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->getStarted()) ? 'STARTED' : 'NOT STARTED (CLASS SETUP?)';
      dpr('GAME ' . $this->id . ': ' . $output);
    }

    return $started;
  }

  /**
   * Determine if a game is finshed (if a winner exists).
   *
   * First time method is called debug code will always show game in progress.
   * This is because the setup() function uses this method.
   */
  public function isFinished() {
    $finished = strlen($this->getWinner()) ? TRUE : FALSE;

    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->getFinished()) ? 'FINISHED: ' . $this->getWinner() : 'IN PROGRESS... (NO WINNER)';
      dpr('GAME ' . $this->id . ': ' . $output);
    }

    return $finished;
  }
}