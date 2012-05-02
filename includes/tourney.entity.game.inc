<?php

/**
 * @file
 * Tournament game class.
 *
 */

class TourneyGame extends TourneyEntity {

  public  $tournamentId = NULL;
  public  $matchId = NULL;
  private $gameMatch = NULL;
  private $gameContestantIds = NULL;
  private $gameContestants = NULL;

  /**
   * Constructor.
   */
  public function __construct($values = array(), $entityType = 'tourney_game') {
    parent::__construct($values, $entityType);
    
    $this->tournamentId = $this->getTournamentId();
    $this->matchId = $this->getMatchId();
        
    // Remove this once I know for sure that modules aren't depending on
    // properties directly on the match object.
    if (variable_get('tourney_backward_compatibility', false)) {
      $this->backwardCompatibility();
    }
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
    // Set the parent match on the game entity.
    $match = tourney_match_load($this->matchId, true);
    $this->match = $match;

    // Load the contestants property on the object.
    foreach ( array(1, 2) as $i ) {
      if (is_object($match) && is_object($match->getContestant($i))) {
        $contestant = $match->getContestant($i);
        $this->contestants[$contestant->eid] = entity_load_single_by_eid($contestant->eid);
      }
    }
  }

  /**
   * Report the match entity_id related to a game.
   *
   * @return array $match_ids
   *  An integer or associative array, keyed by TourneyGame id, of TourneyMatch
   *  ids. Array will be empty if no matches are found.
   */
  public function getMatchId() {
    if ( !property_exists($this, 'id') || is_null($this->id) ) return NULL;
    if ( is_null($this->matchId) ) {
      $query = relation_query('tourney_game', $this->id);
      $query->entityCondition('bundle', 'has_game');
      $results = $query->execute();
      
      if ( !empty($results) ) {
        $relation = array_pop($results);
        $r = relation_load($relation->rid);
        $this->matchId = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
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
  public function getTournamentId() {
    if (is_null($this->tournamentId)) {
      $query = relation_query('tourney_match', $this->getMatchId());
      $query->entityCondition('bundle', 'has_match');
      $results = $query->execute();
      
      if (!empty($results)) {
        $relation = array_pop($results);
        $r = relation_load($relation->rid);
        $this->tournamentId = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
      }
    }
    return $this->tournamentId;
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
  
  /**
   * Lazy load a tournament object this match belongs to.
   */
  public function getMatch() {
    if (is_null($this->gameMatch)) {
      $this->gameMatch = relation_get_related_entity('tourney_game', $this->id, 'has_game', 1);
    }
    return $this->gameMatch;
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
   *   If the $key paramater is NULL then an array of all contestants keyed 
   *   by ["$obj_type.$obj_id"]. If $key parameter is string then only the
   *   element [$key] of all contestants is returned. If $key parameter is an
   *   integer then only the element at offset [$key] is returned.
   */
  public function getContestants($key = NULL) {
    if (is_null($this->gameContestants)) {
      foreach ($this->getContestantIds() as $c_id) {
        $this->gameContestants[$c_id['type'] . '.' . $c_id['id']] = array_pop(entity_load($c_id['type'], array($c_id['id'])));
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
  public function getContestantIds() {
    if (is_null($this->gameContestantIds)) {
      $this->gameContestantIds = TourneyMatch::fetchContestantIds($this->getMatchId());
    }

    return $this->gameContestantIds;
  }

  public function save() {
    parent::save();
    cache_clear_all('tourney_winner_count_' . $this->getMatchId(), 'cache_tourney');
  }

}