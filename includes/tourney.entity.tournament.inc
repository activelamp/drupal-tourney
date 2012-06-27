<?php

/**
 * @file
 * Tournament class.
 */

class TourneyTournamentEntity extends TourneyEntity {

  public  $matchIds = NULL;
  public  $gameIds = NULL;
  public  $uri;
  public  $data;
  public  $contestants = NULL;
  private $tournamentStarted  = NULL;
  private $tournamentFinished = NULL;
  private $tournamentWinner = NULL;
  private $tournamentMatches = NULL;
  public  $status;
  public  $path = array();

  public $tourneyFormatPlugin = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    // Instantiate the plugin, but only if our entity has been created
    // via entity_load(). If our entity has been constructed by another
    // means, like entity_create(), required properties will not yet be
    // available.
    // @todo : Lazy load this?
    if (!empty($this->players)) {
      // Ensure that matches have been created before retreving them.
      // if ($this->matches_created) {
      //   $this->matchIds = $this->getMatchIds();
      //   $this->gameIds = $this->getGameIds();
      // }

      $tourneyFormatPlugin = $this->format;
      if (!class_exists($tourneyFormatPlugin)) {
        throw new Exception(t('Tourney format plugin does not exist'));
      }
      $this->tourneyFormatPlugin = new $tourneyFormatPlugin($this->players, $this);
      // $this->data = $this->tourneyFormatPlugin->structure();

      // Add a url to the object
      $this->uri = $this->getUri();
    }
  }

  /**
   * Static function that returns an array of match ids for that passed in
   * tournament id.
   *
   * @param $tournament_id
   *   The tournament entity id to find matches on.
   * @return array
   *   An array of match entity ids.
   */
  public static function fetchMatchIds($tournament_id) {
    $query = relation_query('tourney_tournament', $tournament_id);
    $query->entityCondition('bundle', 'has_match');
    $query->range(0, 1000); // Override the default RelationQuery class.
    $results = $query->execute();

    $match_ids = array();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $match_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
    }

    return $match_ids;
  }

  /**
   * Static function that returns an array of game ids for that passed in
   * tournament id.
   *
   * @param $tournament_id
   *   The tournament entity id to find matches on.
   * @return array
   *   An array of match entity ids.
   */
  public static function fetchGameIds($tournament_id) {
    $match_ids = TourneyTournamentEntity::fetchMatchIds($tournament_id);

    $game_ids = array();
    foreach ($match_ids as $mid) {
      $game_ids = TourneyMatchEntity::fetchGameIds($mid);
    }

    return $game_ids;
  }

  /**
   * Lazy load all the match ids for this tournament. Once the matches have
   * been loaded for the tournament, it will return the 'cached' copy.
   */
  public function getMatchIds() {
    if (empty($this->matchIds)) {
      $this->matchIds = TourneyTournamentEntity::fetchMatchIds($this->id);
    }
    return $this->matchIds;
  }
  
  /**
   * Retrieve a match by machine name.
   * 
   * @param $mid
   *   The match number to get
   * @return
   *   A fully loaded match object. 
   */
  public function getMatchById($mid) {
    $entity_id = db_query("SELECT id FROM {tourney_match} WHERE name = :name",
      array(':name' => $this->name . '_match-' . $mid))
      ->fetchField();
  
    return $entity_id  > 0 ? tourney_match_load($entity_id) : NULL;
  }
  
  /**
   * Retrieve a match by machine name.
   * 
   * @param $mid
   *   The match number to get
   * @return
   *   The URI to the match as a string. 
   */
  public function getMatchURI($mid) {
    $entity = $this->getMatchById($mid);
    
    return is_object($entity) ? $entity->uri : NULL;
  }

  /**
   * Lazy load all the game ids for this tournament. Once the games have been
   * loaded for the tournament, it will return the 'cached' copy.
   */
  public function getGameIds() {
    if (empty($this->gameIds)) {
      $this->gameIds = TourneyMatchEntity::fetchGameIds($this->getMatchIds()); 
    }
    return $this->gameIds;
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

    // Load all the matches in this tournament.
    $matches = tourney_match_load_multiple($this->getMatchIds());

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
   * Stub code for detemining if a tournament has started
   */
  public function isStarted() {
    // @todo: do something
  }

  /**
   * Report if a tournament is finished.
   *
   * @param TourneyTournamentEntity $tournament
   *
   * @return bool $finished
   *   Will report TRUE if the tournament is finished, FALSE if not.
   */
  function isFinished() {
    $matches = tourney_match_load_multiple($this->getMatchIds());
    if (!empty($matches)) {
      foreach ($matches as $match) {
        // Delegate the checking to the match to see if each match is finished
        if (!$match->isFinished()) {
          return FALSE;
        }
      }
      return TRUE;
    }
    throw new Exception(t('There are no matches for this tournament'));
  }

  /**
   * Set the winner property to the winner of this match.
   */
  public function determineWinner() {
    if ($this->isFinished()) {
      $ranks = $this->fetchRanks();
      $standings = $this->getStandings();

      // todo : remove quick hack, implement custom uasort callback.
      foreach ($standings as $key => $standing) {
        $winners[$key] = $standing['wins'];
      }
      arsort($winners);

      $keys = array_keys($winners);
      $this->winner = $keys[0];
      $this->save();
    }
    else {
      if ($this->winner) {
        $this->winner = NULL;
        $this->save();
      }
    }

    return $this;
  }

  /**
   * Get the path to a tourney url without needing a tourney object loaded.
   */
  public static function path($id) {
    return 'tourney/tournament/' . $id;
  }

  /**
   * Return the uri to the entity object
   */
  public function getUri() {
    return TourneyTournamentEntity::path($this->id);
  }

  /**
   * Delete a tournament.
   */
  public function delete() {
    // Find all the relationships to matches for this tournament.
    $mquery = relation_query('tourney_tournament', $this->id)->entityCondition('bundle', 'has_match');
    $matches = $mquery->execute();

    // Build an array of stuff to delete.
    foreach (array_keys($matches) as $rid) {
     // Get all the match relations and matches and delete them.
     $relation = relation_load($rid);
     relation_delete($rid);
     $match = tourney_match_load($relation->endpoints['und'][1]['entity_id']);
     $match->delete();
    }

    parent::delete();
  }

  /**
  * Save the entity
  */
  public function save() {
    // Store the is_new property in local variable. The property is no longer
    // there after the entity is initially saved.
    $new = property_exists($this, 'is_new') ? $this->is_new : FALSE;
    parent::save();

    // Create the matches and first game if new tournament.
    if ($new) {
      $controller = TourneyController::loadPlugin($this);
      $controller->build();
      $controller->saveMatches();

      // Update the tournament with the number of matches created
      $this->matches_created = count($this->getMatchIds());
      drupal_write_record('tourney_tournament', $this, 'id');
    }
  }

/**
 * Gets a list of participating contestants in seed order from the first round.
 *
 * @return array $contestants
 *   List of contestants, all entities from the first round of a tournament.
 */
  public function getContestants() {
    if (is_null($this->contestants)) {
      $matches = $this->tourneyFormatPlugin->data['matches'];
      
      // Set up two arrays to fill for seed order
      $seed_1 = array();
      $seed_2 = array();
      foreach ($matches as $mid => $match_info) {
        if ($match_info['round'] == 1) {
          // If the match is a bye, get the next match id
          if ($match_info['bye'] == TRUE) {
            $mid = $match_info['nextMatch']['winner'];
          }
          
          $match = $this->tourneyFormatPlugin->getMatchById($mid);
          if (is_object($match)) {
            $seed = 1;
            foreach ($match->getMatchContestants() as $eid => $contestant) {
              $group = "seed_" . $seed++;
              ${$group}[$eid] = $contestant;
            }
          }
        }
      }
      // Reverse seed_2, so we return contestants as 1 2 3 4 8 7 6 5 
      $this->contestants = array_merge($seed_1, array_reverse($seed_2));
    }
    
    return $this->contestants;
  }

/**
 * Pull all matches in and returns a list of all the tallies of wins/losses
 *
 * @return array $standings
 *   Keyed array with wins and losses
 */
  public function getStandings() {
    $matches = $this->getMatchIds();
    $standings = array();
    $i = 0;
    foreach ($matches as $match) {
      $match = entity_load_single('tourney_match', $match);
      // If a match hasn't been won yet, don't count its games
      if ( !$match->getWinner() ) continue;
      foreach ( $match->getContestantIds() as $var => $contestant ) {
        // If the standings array doesn't have this contestant yet, add it
        if ( !array_key_exists($eid = $contestant['eid'], $standings) )
          $standings[$eid] = array('wins' => 0, 'losses' => 0, 'total' => 0, 'first' => -1);
        // Tally either as a winner or a loser, then up the total matches played
        if ( $match->getWinner() == $eid ) {
          $standings[$eid]['wins']++;
        }
        else {
          $standings[$eid]['losses']++;
        }
        $standings[$eid]['total']++;
        // First is used for the rare case of ties, just to see who won first
        if ( $standings[$eid]['first'] == -1 )
          $standings[$eid]['first'] = $i++;
      }
    }
    return $standings;
  }

 /**
  * Retrieve the callback from the plugin for next match and pass and delegate
  * responsibility to the plugin.
  *
  * @param $match
  *   Match object to compare with the internal matchIds property to get its
  *   match placement
  * @param $direction
  *   Either 'winner' or 'loser'
  * @return $match
  *   Match entity of the desired match, otherwise NULL
  */
  public function getNextMatch($match, $direction = 'winner') {
    $plugin =& $this->tourneyFormatPlugin;
    if ( !array_key_exists($match->entity_id, $plugin->flowMap) ) {
      $plugin->flowMap[$match->entity_id] = array();
    }
    if ( !array_key_exists($direction, $plugin->flowMap[$match->entity_id]) ) {
      // Get the callback for this match
      $info = $match->matchInfo;
      if ( !$info ) return NULL;
      if ( !method_exists($plugin, $info['next_match']['callback']) ) return NULL;
      $plugin->flowMap[$match->entity_id][$direction] = $plugin->$info['next_match']['callback']($match, $direction);
    }
    return $plugin->flowMap[$match->entity_id][$direction];
  }

 /**
  * Iterate and cache all the tournament's matches' next matches.
  *
  */
  public function populateFlowMap() {
    $populated = &drupal_static(__FUNCTION__, FALSE);
    $plugin =& $this->tourneyFormatPlugin;
    if ( !$populated ) {
      $matches = entity_load('tourney_match', $this->getMatchIds());
      foreach ( $matches as $match ) {
        foreach ( $plugin->directions as $direction ) {
          $this->getNextMatch($match, $direction);
        }
      }
      $populated = TRUE;
    }
    return $plugin->flowMap;
  }

 /**
  * Get the parent matched to the provided match. Logic will go through
  * and populate the flow map to obtain all next matches and get all
  * matches who lead to the provided.
  *
  * @param $match
  *   Match object
  * @return $match
  *   Array of entities of the desired match
  */
  public function getPreviousMatches($match) {
    $this->populateFlowMap();
    $plugin =& $this->tourneyFormatPlugin;
    $matches = array();
    foreach ( $plugin->flowMap as $pid => $flow ) {
      foreach ( $flow as $dir => $child ) {
        if ( $child && $child->entity_id == $match->entity_id )
          $matches[] = $pid;
      }
    }
    return array_values(entity_load('tourney_match', $matches));
  }
  
  /**
   * Renders the bracket of the tournament.
   */
  public function render() {
    return $this->tourneyFormatPlugin->render();
  }

  /**
   * Function to determine if a match can be edited
   *
   * @param $mid
   *   The match machine name from the plugin
   * @return
   *   Bitwise value with contestant slots turned on (slot 1 in right most position)
   */
  public function canSetContestants($mid) {
    $return = 0;
    $match = $this->getMatchById($mid);
    
    // If no automation is turned on, we should be able to edit both contestants.
    if ($this->get('no_automation', FALSE)) {
      return 0x3;
    }
    if (user_access("edit tourney") == FALSE || count($match->getGameIds()) > 1) {
      return FALSE;
    }
    $round = $match->matchInfo['round'];
    if (!$this->get('show_byes') && $round == 2) {
       $match->findSeedInfo();
    }
    
    // Return a bitwise value with contestant slots turned on (slot 1 in right most position)
    if ($round == 1 && $match->matchInfo['bracket'] === 'main') {
      // Both slots can be set
      $return = 0x3;
    }
    elseif (array_key_exists('seeds', $match->matchInfo)) {
      foreach (array(1,2) as $slot) {
        if (array_key_exists($slot, $match->matchInfo['seeds'])) {
          $return += $slot;
        }
      }
    }
    return $return;
  }

  /**
   * Deprecated function. Use the structure in plugin instead.
   *
   * Return all the matches that have a relation 'has_match' with the passed
   * in tournament object.
   *
   * @param $raw
   *   If set to true only return the matches, do not return brackets and
   *   rounds with it.
   * @param $reset
   *   Boolean to reset the cache.
   * @return
   *   An array of all the rounds and matches.
   */
  public function buildMatches($raw = FALSE, $reset = FALSE) {
    $relations = relation_query('tourney_tournament', $this->id)
      ->entityCondition('bundle', 'has_match')
      ->execute();

    $brackets = array();
    foreach ( $relations as $rid => $relation ) {
      $relation = relation_load($rid);
      $bracket  = $relation->bracket[LANGUAGE_NONE][0]['safe_value'];
      $round    = $relation->round[LANGUAGE_NONE][0]['safe_value'];

      $match    = $relation->endpoints[LANGUAGE_NONE][1]['entity_id'];
      $match    = entity_load_single('tourney_match', $match);

      if ( !array_key_exists($bracket, $brackets) )
        $brackets[$bracket] = array();
      if ( !array_key_exists($round, $brackets[$bracket]) )
        $brackets[$bracket][$round] = array();
      if (is_object($match))
        $brackets[$bracket][$round][$match->entity_id] = $match;
    }

    return $brackets;
  }

  /**
   * Rules called function, usually, to run win code on a match. Calls the plugin
   * specific callback, if there, then checks internally if the tournament's been
   * given a winner
   *
   * @param $match
   *   match object to run win code on
   */
  public function matchIsWon($match) {
    $plugin = $this->tourneyFormatPlugin;
    $info = $match->matchInfo;
    $match->cleanGames();
    $this->determineWinner();
    
    if (!$this->get('no_automation', FALSE)) {
        if (!$match->isFinished()) {
          return;
        }
        // clear winner and loser so we can get a fresh winner, just in case
        $match->clearWinner();
        $winner = $match->getWinnerEntity();
        // odd matches go to top slot, evens to bottom
        $slot = $match->matchInfo['id'] % 2 ? 1 : 2;
        if ($match->nextMatch()) {
          $match->nextMatch()->addContestant($winner, $slot);
        }
    }
  }

  public function get($name, $default = NULL) {
    if (!property_exists($this, 'entity_id')) {
      return isset($this->config[$name]) ? $this->config[$name] : $default;
    }

    global $_tourney_config;
    if (!isset($_tourney_config)) {
      tourney_initialize_configuration();
    }
    
    return !empty($this->id) && !empty($_tourney_config[$this->id][$name]) 
      ? $_tourney_config[$this->id][$name] : $default;
  }

  public function set($name, $value) {
    global $_tourney_config;
    if ( !isset($_tourney_config) ) tourney_initialize_configuration();

    db_merge('tourney')
      ->key(array(
        'id'   => $this->id,
        'type' => $name,
      ))
      ->fields(array(
        'value' => serialize($value)
      ))
      ->execute();

    cache_clear_all('configuration', 'cache_tourney');

    $_tourney_config[$this->id][$name] = $value;
  }
}