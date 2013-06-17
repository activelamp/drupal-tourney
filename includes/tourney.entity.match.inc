<?php

/**
 * @file
 * Tournament match class.
 */

class TourneyMatchEntity extends TourneyEntity {

  public  $tournamentId = NULL;
  public  $gameIds = NULL;
  public  $contestantIds = NULL;
  public  $uri;
  public  $matchInfo = NULL;
  public  $hasByes = NULL;
  // @todo: rename the variable below to $tournament when backwardCompatibility
  // is no longer being used.
  private $matchGames = array();
  private $matchTournament = NULL;
  private $matchWinner = NULL;
  private $matchLoser = NULL;

  public function __construct($values = array(), $entityType = 'tourney_match') {
    parent::__construct($values, $entityType);

    if (property_exists($this, 'id') && $this->id) {
      $this->tournamentId = $this->getTournamentId();
      $this->gameIds = $this->getGameIds();
      // Add a url to the object
      $this->uri = $this->getUri();
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

    $cache_id = is_array($match_id) ? implode('-', $match_id) : $match_id;
    if ( !$cache = cache_get('fetchGameIds_' . $cache_id, 'cache_tourney')) {
      $query = tourney_relation_query('tourney_match', $match_id);
      $query->entityCondition('bundle', 'has_game');
      $results = $query->execute();
      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        $game_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
      }
      sort($game_ids);
      if (count($game_ids) > 0) {
        cache_set('fetchGameIds_' . $cache_id, $game_ids, 'cache_tourney');
      }
    }
    else {
      $game_ids = $cache->data;
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

    if ( !$cache = cache_get('fetchTournamentId_' . $match_id, 'cache_tourney')) {
      $query = tourney_relation_query('tourney_match', $match_id);
      $query->entityCondition('bundle', 'has_match');
      $results = $query->execute();

      if (!empty($results)) {
        $relation = array_pop($results);
        $r = relation_load($relation->rid);
        $tournament_id = $r->endpoints[LANGUAGE_NONE][0]['entity_id'];
      }

      if (isset($tournament_id)) {
        cache_set('fetchTournamentId_' . $match_id, $tournament_id, 'cache_tourney');
      }
    }
    else {
      $tournament_id = $cache->data;
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
    if ($match_id == NULL) {
      if (!isset($this)) return NULL;
      if (!$this->id) return NULL;
      $match_id = $this->id;
    }
    $contestants = array();

    if ( !$cache = cache_get('fetchContestantIds_' . $match_id, 'cache_tourney')) {
      $query = tourney_relation_query('tourney_match', $match_id);
      $query->entityCondition('bundle', 'contestant');
      $results = $query->execute();

      foreach ($results as $relation) {
        $r = relation_load($relation->rid);
        if ( !$r ) return array();
        $type = $r->endpoints[LANGUAGE_NONE][1]['entity_type'];
        $id = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
        $contestants[] = array(
          'type' => $type,
          'id' => $id,
          'eid' => $type . '.' . $id,
          'slot' => $r->slot ? $r->slot[LANGUAGE_NONE][0]['value'] : NULL,
        );
      }
      uasort($contestants, 'TourneyMatchEntity::contestantSort');
      if (count($contestants) > 1) {
        cache_set('fetchContestantIds_' . $match_id, $contestants, 'cache_tourney');
      }
    }
    else {
      $contestants = $cache->data;
    }
    return $contestants;
  }

  private static function contestantSort($a, $b) {
    if ($a['slot'] == $b['slot']) {
      return ($a['id'] < $b['id']) ? -1 : 1;
    }
    return ($a['slot'] < $b['slot']) ? -1 : 1;
  }

  /**
   * Lazy load a tournament object this match belongs to.
   */
  public function getTournament() {
    if (is_null($this->matchTournament)) {
      $this->matchTournament = tourney_tournament_load($this->getTournamentId());
    }
    return $this->matchTournament;
  }

  /**
   * Lazy load a tournament object this match belongs to.
   */
  public function getMatchInfo() {
    if (is_null($this->matchInfo) && is_object($this->getTournament())) {
      // Extract just the match id from the machine name.
      $mid = $this->getMID();

      // Get the plugin data for the match id passed in.
      $tournament = $this->getTournament();
      $this->matchInfo = $tournament->tourneyFormatPlugin->getMatchInfo($mid);
    }
    return $this->matchInfo;
  }

  /**
   * Extract just the match id from the machine name of the entity.
   */
  public function getMID() {
    $tournament = $this->getTournament();
    return substr($this->name, strlen($tournament->name .'_match-'));
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
    $contestants = $this->getMatchContestants();
    if (!$contestants) {
      return NULL;
    }
    $first = reset($contestants);
    if ($first->slot !== NULL) {
      foreach ($contestants as $eid => $contestant) {
        if ($contestant->slot == $slot) {
          return $contestant;
        }
      }
      return NULL;
    }
    $values = array_values($contestants);
    return $values[$slot - 1];
  }

  /**
   * Get the tournament id
   */
  public function getTournamentId() {
    if (is_null($this->tournamentId)) {
      $this->tournamentId = TourneyMatchEntity::fetchTournamentId($this->id);
    }
    return $this->tournamentId;
  }

  /**
   * Get the game ids for this match.
   */
  public function getGameIds() {
    if (is_null($this->gameIds)) {
      $this->gameIds = TourneyMatchEntity::fetchGameIds($this->id);
    }
    return $this->gameIds;
  }

  /**
   * Get the contestant entity type and id for this match.
   */
  public function getContestantIds() {
    if (is_null($this->contestantIds)) {
      $this->contestantIds = TourneyMatchEntity::fetchContestantIds($this->id);
    }
    return $this->contestantIds;
  }

  /**
   * Get the contestant entities
   */
  public function getMatchContestants() {
    $entities = array();

    foreach ($this->getContestantIds() as $contestant) {
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

  /**
   * Clears internal properties to reload winner/loser.
   */
  public function clearWinner() {
    $this->matchWinner = NULL;
    $this->matchLoser = NULL;
  }

  public function getLoser() {
    if (is_null($this->matchLoser)) {
      if (!$this->getWinner()) return NULL;
      foreach ($this->getContestantIds() as $contestant) {
        if ($contestant['eid'] !== $this->getWinner())
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
    if (!$this->getLoser()) {
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
    if ($reset || !$cache = cache_get('tourney_winner_count_' . $this->id, 'cache_tourney')) {
      $games = array();
      $this->getMatchInfo();
      
      if(is_array($this->matchInfo)) {
      if (!array_key_exists('bye', $this->matchInfo) && count($this->getMatchContestants()) < 2) {
        return array();
      }
      }

      foreach ($this->getMatchContestants() as $eid => $contestant) {
        $winners[$eid] = 0;
      }
      foreach ($this->getGames() as $gid => $game) {
        if (!$game->getWinner()) continue;
        $winners[$game->getWinner()]++;
      }

      asort($winners);
      cache_set('tourney_winner_count_' . $this->id, $winners, 'cache_tourney');
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
    $this->getMatchInfo();
    // If this is a bye, it's finished
    if(isset($this->matchInfo)) {
      if (array_key_exists('bye', $this->matchInfo) && $this->matchInfo['bye']) {
        return TRUE;
      }
    }

    $games = tourney_game_load_multiple($this->getGameIds());
    // Had to implement a tally count to double-check for a winner. For some
    // reason, the second match of a match group was saying it was finished
    // before it actually was.
    $tally = array();
    // if empty, it still needs to load games into, therefore is unfinished
    if (empty($games)) {
      return FALSE;
    }
    foreach ($games as $game) {
      // Delegate the checking to the game to see if each game is finished
      if (array_key_exists($game->winner, $tally))
        $tally[$game->winner]++;
      else
        $tally[$game->winner] = 1;
      if (!$game->isFinished()) {
        return FALSE;
      }
    }
    asort($tally);
    $top = array_pop($tally);
    if ($top < $this->games / 2) return FALSE;
    return TRUE;
  }

  /**
   * Set the winner property to the winner of this match.
   */
  public function determineWinner() {
    $this->MatchWinner = NULL;
    if ($this->isFinished()) {
      $ranks = $this->fetchRanks();
      asort($ranks);
      $keys = array_keys($ranks);
      $this->matchWinner = array_pop($keys);
      // @todo: Why are we using matchWinner instead of winner?
      if ($this->winner !== $this->matchWinner) {
        $this->winner = $this->matchWinner;
        // $this->save();
      }
    }
    return $this->matchWinner;
  }

  /**
   * Delete a match and its relations.
   */
  public function delete() {
    // Get all the contestant relations
    $crelations = tourney_relation_query('tourney_match', $this->id)->entityCondition('bundle', 'contestant');
    $crids = $crelations->execute();
    foreach (array_keys($crids) as $rid) {
      relation_delete($rid);
    }

    // Get all the game relations and games
    $gquery = tourney_relation_query('tourney_match', $this->id)->entityCondition('bundle', 'has_game');
    $game_relations = $gquery->execute();

    // Delete the games in this match
    foreach (array_keys($game_relations) as $rid) {
      $relation = relation_load($rid);
      relation_delete($rid);
      $game = tourney_game_load($relation->endpoints['und'][1]['entity_id']);
      if (is_object($game)) {
        $game->delete();
      }
      else {
        drupal_set_message(t('Could not delete game entity %id. Does not exist', array(
         '%id' => $relation->endpoints['und'][1]['entity_id'],
        )), 'warning');
      }
    }

    parent::delete();
  }
  /**
   * Save the entity
   */
  public function save() {
    $this->winner = $this->getWinner();
    if (property_exists($this, 'field_tournament') && !$this->field_tournament[LANGUAGE_NONE]) {
      $tournament = $this->getTournament();
      if ($tournament) {
        $this->field_tournament[LANGUAGE_NONE][0]['target_id'] = $tournament->id;
      }
    }
  
    parent::save();
    // Set the tournament winner if this was the last match.
    if ($this->getTournament()) {
      cache_clear_all('tourney_build_matches_' . $this->getTournament()->id, 'cache_tourney', TRUE);
    }
    cache_clear_all('getHeadToHeadMatchIds:' . $this->getTournament()->id, 'cache_tourney', TRUE);
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
    return TourneyMatchEntity::path($this->id);
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
    if (empty($this->matchGames)) {
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
 * @param integer $entity
 *   Contestant entity to remove.
 */
  public function removeContestant($entity, $save = TRUE) {
    // If we're locked, can't remove the contestant
    if ($this->isLocked()) {
      $message = t('Match is locked, cannot modify contestants.');
      watchdog('tourney', $message);
      return FALSE;
    }
    // Get all the contestants, and, if we haven't got any, we can't remove any!
    $relations = tourney_relation_query('tourney_match', $this->id)->entityCondition('bundle', 'contestant')->execute();
    if (empty($relations)) {
      return;
    }
    foreach ($relations as $relation) {
      $relation = relation_load($relation->rid);
      $rcon = $relation->endpoints[LANGUAGE_NONE][1];
      if (is_object($entity) && $rcon['entity_type'] == $entity->entity_type && $rcon['entity_id'] == $entity->entity_id) {
        relation_delete($relation->rid);
      }
    }
    if ($save) $this->save();
  }

  /**
   * Adds a contestant entity to an object by creating the necessary relation
   *
   * @param Object entity
   *   Any type of entity to fill the target slot with
   * @param integer $slot
   *   Contestant slot to target
   */
  public function addContestant($entity, $slot = NULL) {
    if (!tourney_relation_type_is_target('contestant', $entity->entity_type)) {
      throw new Exception('Invalid entity type. This type is not configured to be a contestant.');
    }

    // If this is round 1, save seed positions.
    if (array_key_exists('seeds', $this->matchInfo) && array_key_exists($slot, $this->matchInfo['seeds'])) {
      $tourney = $this->getTournament();
      $seed_positions = $tourney->get('seed_positions');
      if (!is_array($seed_positions)) {
        $seed_positions = array();
      }
      $seed_positions = array_merge($seed_positions, array($entity->eid => array('weight' => $this->matchInfo['seeds'][$slot])));
      $tourney->set('seed_positions', $seed_positions);
    }

    if (!$this->id) $this->save();

    /* This logic allows contestants that are already on the match to be moved
    without potentially being affected when the copy is removed */
    cache_clear_all('fetchContestantIds_' . $this->id, 'cache_tourney', TRUE);
    cache_clear_all('getContestants:' . $this->tournamentId, 'cache_tourney', TRUE);

    $current_eids = array();
    foreach ($this->getContestantIds() as $contestant) {
      $current_eids[$contestant['eid']] = $contestant['slot'];
    }
    if ( isset($current_eids[$entity->eid]) ) {
      $this->removeContestant($this->getContestant($current_eids[$entity->eid]), FALSE);
    }
    /* */
    if ($slot && $this->getContestant($slot)) {
      $this->removeContestant($this->getContestant($slot), FALSE);
    }
    $entity_keys = array(
      array('entity_type' => 'tourney_match', 'entity_id' => $this->id),
      array('entity_type' => $entity->entity_type, 'entity_id' => $entity->entity_id),
    );
    $rid = relation_insert("contestant", $entity_keys);
    if ($slot) {
      $relation = relation_load($rid);
      $relation->slot[LANGUAGE_NONE][0]['value'] = $slot;
      relation_save($relation);
    }
    $this->contestantIds = NULL;
    return $this;
  }

 /**
  * Get this match's next match contestants go to when they either win or lose
  *
  * @param $direction
  *   Either 'winner' or 'loser'
  * @return $match
  *   Match entity of the desired match, otherwise NULL
  */
  public function nextMatch($direction = 'winner') {
    $this->getMatchInfo();
    if (array_key_exists('nextMatch', $this->matchInfo)
      && array_key_exists($direction, $this->matchInfo['nextMatch'])) {
      $next_plugin_id = $this->matchInfo['nextMatch'][$direction]['id'];
      return $this->getTournament()->getMatchById($next_plugin_id);
    }
    return NULL;
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
    $game = new TourneyGameEntity(array(
      'type'  => $this->getTournament()->get('game_bundle', 'game'),
      'title' => 'Game ' . ++$game_count,
      'name'  => $this->name . '_game-' . ++$game_count,
    ));
    $game->save();
    // Add the relationship to the game.
    $rid = tourney_create_relation('has_game', 'tourney_match', $this->id, 'tourney_game', $game->id);
    cache_clear_all('fetchGameIds_' . $this->id, 'cache_tourney', TRUE);
    return $game;
  }

 /**
  * Iterates through this match's last games and trims the unplayed ones
  */
  public function cleanGames() {
    $this->gameIds = NULL;
    // Check all our games starting from the back to find the games that haven't
    // been won. Since we've already won the match, we're going to remove all
    // the games that no longer serve purpose for the match
    foreach (array_reverse($this->getGameIds()) as $game_id) {
      $game = tourney_game_load($game_id);
      // If we've run across a won game, it's almost certain the rest of the
      // first matches have been played, so we don't need to continue
      if ($game->getWinner()) {
        break;
      }

      // @todo: find a way to just load in a single relation based on two
      // related entities. This code just runs through all the related games to
      // the match to find the specific relation for the game to delete, and
      // removes it.
      $relations = tourney_relation_query('tourney_match', $this->id)
        ->entityCondition('bundle', 'has_game')->execute();

      foreach (array_reverse($relations) as $relation) {
        $relation = relation_load($relation->rid);
        $relation_game = tourney_game_load($relation->endpoints['und'][1]['entity_id']);
        if ($relation_game->id == $game_id) {
          $relation_game->delete();
          relation_delete($relation->rid);
          cache_clear_all('fetchGameIds_' . $this->id, 'cache_tourney', TRUE);
          break;
        }
      }
    }
    $this->gameIds = NULL;
  }

  /**
   * Resets a match so that all the games to the match are deleted, and a fresh
   * new game is added to the match.
   */
  public function resetMatch() {
    $reset_match = &drupal_static(__FUNCTION__, TRUE);
    foreach ($this->getGameIds() as $game_id) {
      // Unset the winners so that self::cleanGames can delete them.
      $game = tourney_game_load($game_id);
      if (is_object($game)) {
        $game->winner = NULL;
        $game->save();
      }
    }
    $this->addGame();
    $this->winner = "";
    $this->save();
    drupal_static_reset(__FUNCTION__);
  }

 /**
  * Checks this match's games to determine if they've all been won
  *
  * @return $passed
  *   Boolean of whether all the games have been won
  */
  public function checkGames() {
    foreach ($this->getGameIds() as $game_id) {
      $game = tourney_game_load($game_id);
      if (!$game->getWinner()) {
        return FALSE;
      }
    }
    return TRUE;
  }
}