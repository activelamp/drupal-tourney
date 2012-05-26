<?php

/**
 * @file
 * Tournament class.
 */

class TourneyTournament extends TourneyEntity {

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
      if ($this->matches_created) {
        $this->matchIds = $this->getMatchIds();
        $this->gameIds = $this->getGameIds();
      }
      
      $tourneyFormatPlugin = $this->format;
      if (!class_exists($tourneyFormatPlugin)) {
        throw new Exception(t('Tourney format plugin does not exist'));
      }   
      $this->tourneyFormatPlugin = new $tourneyFormatPlugin($this);
      $this->data = $this->tourneyFormatPlugin->structure();
      
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
    $match_ids = TourneyTournament::fetchMatchIds($tournament_id);
    
    $game_ids = array();
    foreach ($match_ids as $mid) {
      $game_ids = TourneyMatch::fetchGameIds($mid);
    }

    return $game_ids;
  }

  /**
   * Lazy load all the match ids for this tournament. Once the matches have 
   * been loaded for the tournament, it will return the 'cached' copy.
   */
  public function getMatchIds() {
    if (empty($this->matchIds)) {
      $this->matchIds = TourneyTournament::fetchMatchIds($this->id); 
    }
    return $this->matchIds;
  }
  
  /**
   * Lazy load all the game ids for this tournament. Once the games have been
   * loaded for the tournament, it will return the 'cached' copy.
   */
  public function getGameIds() {
    if (empty($this->gameIds)) {
      $this->gameIds = TourneyMatch::fetchGameIds($this->getMatchIds()); 
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
   * Determine if a tournament is finished (when all matches are finished).
   *
   * @return bool
   *   Return TRUE if the tournament is finished.
   */
  public function isFinished() {
    if (is_null($this->tournamentFinished)) {
      $this->tournamentFinished = $this->tourneyFormatPlugin->isFinished($this);
    }

    return $this->tournamentFinished;
  }

  /**
   * Set the winner property to the winner of this match.
   */
  public function determineWinner() {
    if ($this->isFinished()) {
      $this->tourneyFormatPlugin->determineWinner($this);
    }
    else {
      if ( $this->winner ) {
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
    return TourneyTournament::path($this->id);
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
    // @todo: Change the is_new property if this entity patch gets in:
    //   Patch at - http://drupal.org/node/1558266
    $new = property_exists($this, 'is_new') ? $this->is_new : FALSE;
    parent::save();
    
    // Create the matches and first game if new tournament.
    if ($new) {
      $controller = new TourneyController($this);
      $controller->createMatches();
      
      // Update the tournament with the number of matches created
      $this->matches_created = count($this->getMatchIds());
      drupal_write_record('tourney_tournament', $this, 'id');
    }
  }

/**
 * Gets a list of participating contestants in seed order
 *
 * @return array $contestants 
 *   List of contestants, all entities
 */
  public function getContestants() {
    return $this->tourneyFormatPlugin->getContestants();
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
    // Get the callback for this match
    $info = $match->matchInfo;
    if ( !$info ) return NULL;
    if ( !method_exists($this->tourneyFormatPlugin, $info['next_match']['callback']) ) return NULL;
    return $this->tourneyFormatPlugin->$info['next_match']['callback']($match, $direction);
  }

  public function getPreviousMatches($match, $direction = 'winner') {
    // Get the callback for this match
    $info = $match->matchInfo;
    if ( !$info ) return NULL;
    if ( !method_exists($this->tourneyFormatPlugin, $info['previous_matches']['callback']) ) return NULL;
    return $this->tourneyFormatPlugin->$info['previous_matches']['callback']($match, $direction);
  }
  
  public function render() {
    $info = $this->tourneyFormatPlugin->getPluginInfo(get_class($this->tourneyFormatPlugin));
    
    // Build a classes array for the tournament wrapper
    $classes_array = array('tourney', "tourney-" . $info['machine name']);
    $classes_array[] = variable_get('tourney_hide_byes', TOURNEY_HIDE_BYES) 
        ? 'hide-byes' : '';
    
    $output = "<div class='". join(" ", $classes_array)."'><div class='tourney-inner'>";
    $output .= $this->tourneyFormatPlugin->render();
    $output .= "</div></div>";
    return $output;
  }

  public function canSetContestants($match) {
    if (user_access("edit tourney") == FALSE) return FALSE;
    
    if (variable_get('tourney_hide_byes', TOURNEY_HIDE_BYES)
      && $match->matchInfo['round']['id'] == 2) {
       $match->findSeedInfo(); 
    }
      
    return ($match->matchInfo['round']['id'] == 1) || array_key_exists('seed_position', $match->matchInfo);
  }
  
  /**
   * Deprecated function. Use the structure in plugin instead.
   * 
   * Return all the matches that have a relation 'has_match' with the passed in
   * tournament object.
   *
   * @param $raw
   *   If set to true only return the matches, do not return brackets and rounds
   *   with it.
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
}