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
  public  $uri;
  public  $roundInfo = NULL;
  public  $bracketInfo = NULL;
  // @todo: rename the variable below to $tournament when backwardCompatibility
  // is no longer being used.
  private $matchTournament = NULL;
  private $matchGames = NULL;
  private $matchStarted = NULL;
  private $matchFinished = NULL;
  private $matchWinner = NULL;
  private $matchLoser = NULL;
  private $matchSetup = NULL;

  public function __construct($values = array(), $entityType = 'tourney_match') {
    parent::__construct($values, $entityType);

    if (property_exists($this, 'id') && $this->id) {
      $this->tournamentId = $this->getTournamentId();
      $this->gameIds = $this->getGameIds();
      // Add a url to the object
      $this->uri = $this->getUri();
      
      // @todo: Do we really want to load this on instanstiation
      $this->roundInfo = $this->getRoundInfo();
    }    
    
    // Remove this once I know for sure that modules aren't depending on
    // properties directly on the match object.
    if (variable_get('tourney_backward_compatibility', false)) {
      $this->backwardCompatibility();
    }
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
  public static function fetchContestantIds($match_id = NULL) {
    if ( $match_id == NULL ) $match_id = $this->id;
    $contestants = array();

    $query = relation_query('tourney_match', $match_id);
    $query->entityCondition('bundle', 'contestant');
    $results = $query->execute();

    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $type = $r->endpoints[LANGUAGE_NONE][1]['entity_type'];
      $id = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
      $contestants[] = array(
        'type' => $type,
        'id' => $id,
        'eid' => $type .  '.' . $id,
        'slot' => $r->slot ? $r->slot[LANGUAGE_NONE][0]['value'] : NULL,
      );
    }
    uasort($contestants, 'TourneyMatch::contestantSort');
    
    return $contestants;
  }

  private static function contestantSort($a, $b) {
    if ( $a['slot'] == $b['slot'] ) return ( $a['id'] < $b['id'] ) ? -1 : 1;
    return ( $a['slot'] < $b['slot'] ) ? -1 : 1;
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
   * Lazy load a tournament object this match belongs to.
   */
  public function getRoundInfo() {
    if (is_null($this->roundInfo) && is_object($this->getTournament()->tourneyFormatPlugin)) {
      $this->roundInfo = $this->getTournament()->tourneyFormatPlugin->getRoundInfo($this);
    }
    return $this->roundInfo;
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
    $contestants = $this->getContestants();
    if ( !$contestants ) return NULL;
    $first = reset($contestants);
    if ( $first->slot !== NULL ) {
      foreach ( $contestants as $eid => $contestant ) {
        if ( $contestant->slot == $slot ) return $contestant;
      }
      return NULL;
    }
    $values = array_values($contestants);
    return $contestants[$values[$slot-1]];
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
   * Get the contestant entities
   */
  public function getContestants() {
    $entities = array();
    foreach ( $this->getContestantIds() as $contestant ) {
      $entity = entity_load_single_by_eid($contestant['eid']);
      $entities[$entity->eid] = $entity;
      $entities[$entity->eid]->slot = $contestant['slot'];  
    }
    return $entities;  
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

  public function getLoser() {
    if ( is_null($this->matchLoser) ) {
      if ( !$this->getWinner() ) return NULL;
      foreach ( $this->getContestantIds() as $contestant ) {
        if ( $contestant['eid'] !== $this->getWinner() ) 
          $this->matchLoser = $contestant['eid'];
      }
    }
    return $this->matchLoser;
  }

  /**
   * Helper getters
   */
  public function getWinnerEntity() {
    if (!$this->getWinner()) {
      return NULL;
    }
    return entity_load_single_by_eid($this->getWinner());
  }
  
  public function getLoserEntity() {
    if ( !$this->getLoser() ) {
      return NULL;
    }
    return entity_load_single_by_eid($this->getLoser());
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

      if ( count($this->getContestants()) < 2 ) return array();

      foreach ( $this->getContestants() as $eid => $contestant ) {
        $winners[$eid] = 0;
      }
      foreach ( $this->getGames() as $gid => $game ) {
        if ( !$game->getWinner() ) continue;
        $winners[$game->getWinner()]++;
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
    // Had to implement a tally count to double-check for a winner. For some reason, the second match of a match group was saying it was finished before it actually was
    $tally = array();
    if (!empty($games)) {
      foreach ($games as $game) {
        // Delegate the checking to the game to see if each game is finished
        if ( array_key_exists($game->winner, $tally) ) 
          $tally[$game->winner]++;
        else
          $tally[$game->winner] = 1;
        if (!$game->isFinished()) {
          return FALSE;
        }
      }
      asort($tally);
      $top = array_pop($tally);
      if ( $top < $this->games / 2 ) return FALSE;
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
      $this->matchWinner = array_pop($keys);
    }
    return $this->matchWinner;
  }
  
  /**
   * Delete a match and its relations.
   */
  public function delete() {
    // Get all the contestant relations
    $crelations = relation_query('tourney_match', $this->id)->entityCondition('bundle', 'contestant');
    $crids = $crelations->execute();
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
      $this->gameIds = NULL;
      $this->getTournament()->determineWinner();
      if (method_exists($this, 'getTournament')) {
        cache_clear_all('tourney_build_matches_' . $this->getTournament()->id, 'cache_tourney', TRUE);
      }
    }
  }
  
  /**
    * Get the path to a tourney url without needing a tourney object loaded.
    */
  public static function path($id) {
    return 'tourney/match/' . $id;
  }

  /**
   * Return the uri to the entity object
   */
  public function getUri() {
    return TourneyMatch::path($this->id);
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

  /**
   * Checks the next matches to determine whether or not either has been started.
   * If they have, the current match shouldn't be editable
   *
   * @return $locked
   *   Returns a bool detemining whether the current match can be edited
   */
  public function isLocked() {
    // Get the next matches...
    if ($this->getWinnerEntity() && $this->nextMatch('winner')) {
      if (count($this->nextMatch('winner')->getGameIds()) > 1) return TRUE;
    }
    if ($this->getLoserEntity() && $this->nextMatch('loser')) {
      if (count($this->nextMatch('loser')->getGameIds()) > 1) return TRUE;
    }
      
    return FALSE;
  }

  public function canSetContestants() {
    return $this->getTournament()->canSetContestants($this);
  } 

/**
 * Cleanly removes an existing contestant from a match, deleting the relation
 *
 * @param integer $slot
 *   Contestant slot to target
 */
  public function removeContestant($entity) {
    // If we're locked, can't remove the contestant
    if ( $this->isLocked() ) {
      $message = t('Match is locked, cannot modify contestants.');
      watchdog('tourney', $message);
      return FALSE;
    }
    // Get all the contestants, and, if we haven't got any, we can't remove any! 
    $relations = relation_query('tourney_match', $this->id)->entityCondition('bundle', 'contestant')->execute();
    if ( empty($relations) ) return;
    foreach ( $relations as $relation ) {
      $relation = relation_load($relation->rid);
      $rcon = $relation->endpoints[LANGUAGE_NONE][1];
      if ( $rcon['entity_type'] == $entity->entity_type && $rcon['entity_id'] == $entity->entity_id )        
        relation_delete($relation->rid);
    }
    $this->save();
    // Until there's a better way for addressing specific contestant names, using this:
    // Pull in the relation so we can delete it, then apply changes to the match object
    //relation_delete(array_pop($relations)->rid);
    //unset($this->contestantIds[$relation_type]);
    //$this->save();
  }

/**
 * Automatically populate the next matches to this match with the necessary 
 * contestants by adding the relation
 *
 * @param integer $slot
 *   Contestant slot to target
 * @param Object entity
 *   Any type of entity to fill the target slot with
 */
  public function setContestant($slot, $entity) {
    // Get the current contestant in the target slot
    $current = $this->getContestant($slot);
    // If it's empty, no need to worry
    if ( $current !== NULL ) {
      // If it isn't, however... We'll need to check if it's same. If not, remove it.
      if ( $current->eid == $entity->eid ) return;
      $this->removeContestant($current);
    }
    // Create the relation...
    $entity_keys = array(
      array('entity_type' => 'tourney_match', 'entity_id' => $this->id),
      array('entity_type' => $entity->entity_type, 'entity_id' => $entity->entity_id),
    );
    // ... and save it!
    $rid = relation_insert("contestant_$slot", $entity_keys);
    $this->contestantIds = NULL;
  }

/**
 * Adds a contestant entity to an object by creating the necessary relation
 *
 * @param integer $slot
 *   Contestant slot to target
 * @param Object entity
 *   Any type of entity to fill the target slot with
 */
  public function addContestant($entity, $slot = NULL) {
    //@todo logic to remove existing conflicting contestant(s)
    $entity_keys = array(
      array('entity_type' => 'tourney_match', 'entity_id' => $this->id),
      array('entity_type' => $entity->entity_type, 'entity_id' => $entity->entity_id),
    );
    $rid = relation_insert("contestant", $entity_keys);
    if ( $slot ) {
      $relation = relation_load($rid);
      $relation->slot[LANGUAGE_NONE][0]['value'] = $slot;
      relation_save($relation);
    }
    $this->contestantIds = NULL;
  }

/**
 * Automatically populate the next matches to this match with the necessary contestants 
 */
  public function moveContestants() {
    // If this match hasn't been won, we shouldn't be doing anything with the 
    // next matches.
    if ( !$this->isFinished() ) return;
    // Set the matchWinner back to NULL so it has to be reloaded
    $this->matchWinner = NULL;
    // Instantiate the two origin contestants
    $winner = NULL;
    $loser  = NULL;
    // This loop goes through 'all' the contestants and fills them in the 
    // appropriate winner/loser variables. Since we've only got two contestants,
    // it's just a glorified abstraction in case we have more contestants later
    foreach ( $this->getContestantIds() as $slot => $contestant ) {
      $entity = entity_load_single($contestant['type'], $contestant['id']);
      if ($this->getWinner() == $contestant['eid']) {
        $winner = $entity;
      }
      else {
        $loser = $entity;
      }
    }
    // Another code-saving abstraction, use the handy new setContestant to easily 
    // apply the entities to their targets
    if ($this->nextMatch()) {
      $this->nextMatch()->addContestant($winner);
    }
    
    if ($this->nextMatch('loser')) {
      $this->nextMatch('loser')->addContestant($loser);
    } 
  }

 /**
  * Get this match's next match contestants go to when they either win or lose
  *
  * @param $direction
  *   Either 'winner' or 'loser'
  * @return $place
  *   Match placement of the desired match, otherwise NULL 
  */
  public function nextMatch($direction = 'winner') {
    return $this->getTournament()->getNextMatch($this, $direction);
  }

 /**
  * Adds an empty game to this match
  *
  * @return $game
  *   Newly created game entity 
  */
  public function addGame() {
    $this->cleanGames();
    $game_count = count($this->getGameIds());
    $game = new TourneyGame(array(
      'type'  => 'game',
      'title' => 'Game ' . ++$game_count,
      'name'  => $this->name . '_game-' . ++$game_count,
    ));
    $game->save();
    // Add the relationship to the game.
    $rid = tourney_create_relation('has_game', 'tourney_match', $this->id, 'tourney_game', $game->id);
    return $game;
  }

 /**
  * Iterates through this match's last games and trims the unplayed ones
  */
  public function cleanGames() {
    $this->gameIds = NULL;
    // Check all our games starting from the back to find the games that haven't been won
    // Since we've already won the match, we're going to remove all the games that no longer serve purpose for the match
    foreach ( array_reverse($this->getGameIds()) as $game_id ) {
      $game = tourney_game_load($game_id);
      // If we've run across a won game, it's almost certain the rest of the first matches have been played, so we don't need to continue
      if ( $game->getWinner() ) break;
      // @todo: find a way to just load in a single relation based on two related entities
      // This code just runs through all the related games to the match to find the specific relation for the game to delete, and removes it
      $relations = relation_query('tourney_match', $this->id)->entityCondition('bundle', 'has_game')->execute();
      foreach ( array_reverse($relations) as $relation ) {
        $relation = relation_load($relation->rid); 
        $relation_game = tourney_game_load($relation->endpoints['und'][1]['entity_id']);
        if ( $relation_game->id == $game_id ) {
          $relation_game->delete();
          relation_delete($relation->rid);
          break;
        }
      }
    }
    $this->gameIds = NULL;
  }

 /**
  * Checks this match's games to determine if they've all been won
  *
  * @return $passed
  *   Boolean of whether all the games have been won 
  */
  public function checkGames() {
    foreach ( $this->getGameIds() as $game_id ) {
      $game = tourney_game_load($game_id);
      if ( !$game->getWinner() ) {
        return FALSE;
      }
    } 
    return TRUE;
  } 
}