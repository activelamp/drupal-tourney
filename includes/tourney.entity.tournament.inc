<?php

/**
 * @file
 * Tournament class.
 */

class TourneyTournament extends TourneyEntity {

  public  $matchIds = NULL;
  public  $gameIds = NULL;
  public  $uri;
  private $tournamentStarted  = NULL;
  private $tournamentFinished = NULL;
  private $tournamentWinner = NULL;
  private $tournamentMatches = NULL;

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
      
      if (!class_exists($this->format)) {
        throw new Exception(t('Tourney format plugin does not exist'));
      }   
      $this->tourneyFormatPlugin = new $this->format($this->players);
      
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
  function buildMatches($raw = FALSE, $reset = FALSE) {
    if ($reset || !$cache = cache_get('tourney_build_matches_' . $this->id, 'tourney_cache')) {
      $query = relation_query('tourney_tournament', $this->id);
      $results = $query->entityCondition('bundle', 'has_match')
                        ->range(0, $this->matches_created)
                        ->execute();

      $matches = $brackets = array();
      foreach ($results as $id => $relation) {
        $relation = relation_load($id);
        $bracket = field_get_items('relation', $relation, 'bracket');
        $round = field_get_items('relation', $relation, 'round');
        $endpoints = field_get_items('relation', $relation, 'endpoints');

        $match = tourney_match_load($endpoints[1]['entity_id']);
        $matches[] = $match;

        if ($bracket[0]['safe_value'] && $round[0]['safe_value']) {
          $brackets[$bracket[0]['safe_value']][$round[0]['safe_value']][$match->id] = array(
            'match' => $match,
            'games' => $match->getGames(),
          );
        }
        elseif ($round[0]['safe_value']) {
          $brackets[$round[0]['safe_value']][$match->id] = array(
            'match' => $match,
            'games' => $match->getGames(),
          );
        }
        else {
          $brackets[$match->id] = array(
            'match' => $match,
            'games' => $match->getGames(),
          );
        }
      }

      cache_set('tourney_build_matches_' . $this->id . '_raw', $matches, 'cache_tourney');
      cache_set('tourney_build_matches_' . $this->id, $brackets, 'cache_tourney');
      return $raw ? $matches : $brackets;
    }
    $cache_raw = cache_get('tourney_build_matches_' . $this->id . '_raw', 'cache_tourney');
    return $raw ? $cache_raw->data : $cache->data;

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
    // Store the property in local variable.  The property is no longer there
    // after the entity is initially saved.
    // @todo: Change the is_new property if this entity patch gets in:
    //   Patch at - http://drupal.org/node/1558266
    $new = property_exists($this, 'is_new') ? $this->is_new : FALSE;
    parent::save();
    
    // Set the uri
    
    // Create the matches and first game if new tournament.
    if ($new) {
      // Build a nested array of matches for the tournament being saved.
      $matches = new TourneyFactory($this->players, $this->format);
      $matches->create();
      // Create the actual match entities and the first game.
      $matches_created = $matches->save($this);
      // Update the tournament with the number of matches created
      $this->matches_created = $matches_created;
      drupal_write_record('tourney_tournament', $this, 'id');
    }
  }


/**
 * Goes through the tournament's matches to return one by name
 *
 * @param string $name
 *   Identifier of the match, $match->this_match
 * @return TourneyMatch $match 
 *   The desired match
 */
  public function getMatchByName($name) {
    if ($name == 'manual') {
      return NULL;
    }
    
    // Seperate the parts of the match path.
    $parts = explode('_', $name);

    // Get just the structure of the next match
    $matches = $this->buildMatches();
    $context = $matches[$parts[0]][$parts[1]];

    // Set $id to the round number
    $id = tourney_get_name_part($name, 'match');
    
    /**
     * The bracket structure should be built in the order of matches. This for
     * loop assumes that the matches will always be in order of match-1, match-2,
     * and so on.
     * 
     * Create an iterator for each match, and return the match that has the same
     * match number as the iterator.
     */
    $i = 1;
    foreach($context as $match) {
      if ($id == $i) {
        return $match['match'];
      }
      $i++;
    }
  }

/**
 * Gets a list of participating contestants in seed order
 *
 * @return array $contestants 
 *   List of contestants, all entities
 */
  public function getContestants() {
    // Take matches from only the first round, since those are the manually populated ones
    $bracket = array_shift($this->buildMatches());
    $round = $bracket['round-1'];
    // Set up two arrays to fill for seed order
    $seed_1 = array();
    $seed_2 = array();
    foreach ( $round as $match ) {
      $match = $match['match'];
      foreach ( $match->getContestantIds() as $var => $contestant ) {
        $slot = substr($var, strlen($var)-1);        
        // Fill the seeds by group, contestant_1 to seed_1, contestant_2 to seed_2
        $group = "seed_$slot";
        $contestant = $match->getContestant($slot);
        ${$group}[$contestant->eid] = $contestant;
      }
    }
    // Reverse seed_2, so we return contestants as 1 2 3 4 8 7 6 5 
    $contestants = array_merge($seed_1, array_reverse($seed_2));
    return $contestants;
  }

/**
 * Pull all matches in and returns a list of all the tallies of wins/losses 
 *
 * @return array $standings 
 *   Keyed array with wins and losses
 */
  public function getStandings() {
    $matches = $this->buildMatches(TRUE);
    $standings = array();
    $i = 0;
    foreach ($matches as $match) {
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
}