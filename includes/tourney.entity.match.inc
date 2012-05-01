<?php

/**
 * @file
 * Tournament match class.
 */

class TourneyMatch extends TourneyEntity {

  public  $tournamentId = NULL;
  public  $gameIds = NULL;
  public  $contestantIds = NULL;
  public  $contestant1 = NULL;
  public  $contestant2 = NULL;
  // @todo: rename the variable below to $tournament when backwardCompatibility
  // is no longer being used.
  private $matchTournament = NULL;
  private $matchGames = NULL;
  private $matchStarted = NULL;
  private $matchFinished = NULL;
  private $matchWinner = NULL;
  private $matchSetup = NULL;

  public function __construct($values = array(), $entityType = 'tourney_match') {
    parent::__construct($values, $entityType);

    $this->tournamentId = $this->getTournamentId();
    $this->gameIds = $this->getGameIds();
    
    // Remove this once I know for sure that modules aren't depending on
    // properties directly on the match object.
    if (variable_get('tourney_backward_compatibility', false)) {
      $this->backwardCompatibility();
    }
    $this->checkContestants();
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
  public static function fetchTournamentId($match_id) {
    $tournament_id = NULL;
    
    $query = relation_query('tourney_match', $match_id);
    $query->entityCondition('bundle', 'has_match');
    $results = $query->execute();
    
    if (!empty($results)) {
      $relation = array_pop($results);
      $r = relation_load($relation->rid);
      $tournament_id = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
    }
    return $tournament_id;
  }
  
  /**
   * Report the contestants related to a match.
   *
   * @param integer $match_id
   *
   * @return array $contestants
   *   An associative array containing:
   *   - contestant: An array of entity type concatenated with entity id:
   *     - type: String denoting the entity type.
   *     - id: Integer denoting the specific entity id.
   *   An empty array will be returned if no contestants are found.
   */
  public static function fetchContestantIds($match_id) {
    $contestants = array();

    $query = relation_query('tourney_match', $match_id);
    $query->entityCondition('bundle', array(TOURNEY_CONTESTANT_1, TOURNEY_CONTESTANT_2), 'IN');
    $results = $query->execute();
    
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $type = $r->endpoints[LANGUAGE_NONE][1]['entity_type'];
      $id = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
      
      $contestants[$r->relation_type] = array(
        'type' => $type,
        'id' => $id,
      );
    }
    
    return $contestants;
  }
  
  /**
   * For backward compatibility I'm loading public properties on the object 
   * that used to be in hook_entity_load().
   * 
   * The tourney module does not have any dependencies on these properties. 
   * This is only here in case Riot is expecting these properties on the object
   * and is not using one of the public API methods to retrieve property.
   */
   private function backwardCompatibility() {
     // Get the contestants in this match on the object.
     $this->contestant1 = $this->getContestant(1);
     $this->contestant2 = $this->getContestant(2);
     
     // Put the tournament object on the object
     $this->tournament = $this->getTournament();
     
     // Load the game ids onto the object
     $this->games_played = $this->getGameIds();
   }

  /**
   * Lazy load a tournament object this match belongs to.
   */
  public function getTournament() {
    if (is_null($this->matchTournament)) {
      $this->matchTournament = relation_get_related_entity('tourney_match', $this->id, 'has_match', 1);
    }
    return $this->matchTournament;
  }
  
  /**
   * Get a contestant on the match.
   * 
   * @param int $slot
   *   The contestant slot number to get a contestant from.
   */
  public function getContestant($slot) {
    if (!is_numeric($slot)) {
      throw new Exception(t('$slot must be a number'));
    }
    $contestants = $this->getContestantIds();
    
    if (!empty($contestants) && !empty($contestants['contestant_'. $slot])) {
      return array_pop(entity_load($contestants['contestant_'. $slot]['type'], array($contestants['contestant_'. $slot]['id'])));
    }
    $var = "contestant$slot";
    return $this->$var;
  }
  
  /**
   * Get the tournament id
   */
  public function getTournamentId() {
    if (is_null($this->tournamentId)) {
      $this->tournamentId = TourneyMatch::fetchTournamentId($this->id);
    }
    return $this->tournamentId;
  }
  
  /**
   * Get the game ids for this match.
   */
  public function getGameIds() {
    if (is_null($this->gameIds)) {
      $this->gameIds = TourneyMatch::fetchGameIds($this->id);
    }
    return $this->gameIds;
  }
  
  /**
   * Get the contestant entity type and id for this match.
   */
  public function getContestantIds() {
    if (is_null($this->contestantIds)) {
      $this->contestantIds = TourneyMatch::fetchContestantIds($this->id);
    }
    return $this->contestantIds;
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
   * @param $reset
   *   Flag to rebuild the cache.
   *
   * @return array
   *   Returns an array keyed by uid with value being the number of won games
   *   sorted in descending order.
   */
  public function fetchRanks($reset = FALSE) {
    if ($reset || !$cache = cache_get('tourney_winner_count_' . $this->id, 'tourney_cache')) {
      $games = array();

      // If there aren't two contestants in the match return
      if (empty($this->getContestant(1)->_entity_type) || empty($this->getContestant(2)->_entity_type)) {
        return array();
      }
      
      $contestant1 = $this->getContestant(1);
      $contestant2 = $this->getContestant(2);
      // Set up the default count array.
      $winners = array(
        $contestant1->_entity_type . '.' . $contestant1->_entity_id => 0,
        $contestant2->_entity_type . '.' . $contestant2->_entity_id => 0,
      );

      foreach ($this->getGameIds() as $entity_id) {
        $games[$entity_id] = tourney_game_load($entity_id);
        // Check to make sure the game has a winner
        if ($games[$entity_id]->winner) {
          // Tally up the winners in array keyed by uid
          $winners[$games[$entity_id]->winner] =
            array_key_exists($games[$entity_id]->winner, $winners)
            ? ++$winners[$games[$entity_id]->winner] : 1;
        }
      }
      asort($winners);
      cache_set('tourney_winner_count_' . $this->id, $winners, 'tourney_cache');
      return $winners;
    }
    return !empty($cache->data) ? $cache->data : array();
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

    return $started;
  }

  /**
   * Determine if match is finished (when all games are finished).
   * 
   * @return bool
   */
  public function isFinished() {
    $games = tourney_game_load_multiple($this->getGameIds());
    if (!empty($games)) {
      foreach ($games as $game) {
        // Delegate the checking to the game to see if each game is finished
        if (!$game->isFinished()) {
          return FALSE;
        }
      }
      return TRUE;
    }
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
  
  /**
   * Delete a match and its relations.
   */
  public function delete() {
    // Get all the contestant relations
    $crids = array();
    $c1q = relation_query('tourney_match', $this->id)->entityCondition('bundle', TOURNEY_CONTESTANT_1);
    $crids += $c1q->execute();
    $c2q = relation_query('tourney_match', $this->id)->entityCondition('bundle', TOURNEY_CONTESTANT_2);
    $crids += $c2q->execute();
    foreach (array_keys($crids) as $rid) {
      relation_delete($rid);
    }
    
    // Get all the game relations and games
    $gquery = relation_query('tourney_match', $this->id)->entityCondition('bundle', 'has_game');
    $game_relations = $gquery->execute();
    
    // Delete the games in this match
    foreach (array_keys($game_relations) as $rid) {
      $relation = relation_load($rid);
      relation_delete($rid);
      $game = tourney_game_load($relation->endpoints['und'][1]['entity_id']);
      $game->delete();
    }
    
    parent::delete();
  }
  /**
   * Save the entity
   */
  public function save() {
    parent::save();
    // Set the tournament winner if this was the last match.
    if ( $this->getTournament() ) {
      if ( $this->getTournament()->isFinished() ) {
        if ( !$this->getTournament()->winner ) {
          $this->getTournament()->determineWinner();
        }
      }
    }
    $this->checkContestants();
    $this->updateContestants();
    if (method_exists($this, 'getTournament')) {
      cache_clear_all('tourney_build_matches_' . $this->getTournament()->id, 'cache_tourney', TRUE);
    }
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
      foreach ($this->getGameIds() as $gameId) {
        $this->matchGames[$gameId] = tourney_game_load($gameId);
      }
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

  function checkContestants() {
    foreach ( $this->getContestantIds() as $variable => $contestant ) {
      $id = $variable[strlen($variable)-1];
      $relation_contestant = relation_get_related_entity('tourney_match', $this->id, $variable);
      $match_contestant = $this->getContestant($id);
      if ( $relation_contestant == $match_contestant ) continue;
      $relations = relation_query('tourney_match', $this->id)->entityCondition('bundle', $id)->execute();
      relation_delete(array_pop($results)->rid);
    }
  }

  function updateContestants() {
    $contestants = array('contestant_1' => $this->contestant1, 'contestant_2' => $this->contestant2);
    foreach ( $contestants as $variable => $contestant ) {
      $id = $variable[strlen($variable)-1];
      $relation_contestant = relation_get_related_entity('tourney_match', $this->id, $variable);
      $match_contestant = $this->getContestant($id);
      if ( !is_object($relation_contestant) && !empty($match_contestant) ) {
        if ( is_object($match_contestant) ) {
          $contestant_type = $match_contestant->_entity_type;
          $contestant_id = $match_contestant->_entity_id;
        }
        else {
          list($contestant_type, $contestant_id) = explode('.', $match_contestant);
        }
        $entity_keys = array(
          array('entity_type' => 'tourney_match', 'entity_id' => $this->id),
          array('entity_type' => $contestant_type, 'entity_id' => $contestant_id),
        );
        $rid = relation_insert($variable, $entity_keys);
      }
    }
  }
}