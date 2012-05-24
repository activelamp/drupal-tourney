<?php

/**
 * @file
 * Provides a controller to build matches after a tournament entity is created.
 */

// Maximum number of rounds that an elimination tournament will allow.
define('MAXIMUM_ROUNDS', 5);

/**
 * A Default factory for building matches and the first game of a match for a
 * tourney tournament.
 */
class TourneyController {
  private static $controller = null;
  protected $tourneyFormatPlugin;
  protected $matches;
  protected $tournament;
  protected $pluginInfo;

  /**
   * Constructor
   */
  public function __construct($tournament = NULL) {
    if (!is_null($tournament)) {
      $this->tournament = $tournament;
      $tourneyFormatPlugin = $tournament->format;

      if (!class_exists($tourneyFormatPlugin)) {
        trigger_error(check_plain(t('Unable to create !tourneyFormatPlugin matches. Class not found.', array('!tourneyFormatPlugin' => $tourneyFormatPlugin))), E_USER_ERROR);
      }

      $interfaces = class_implements($tourneyFormatPlugin);
      if (isset($interfaces['TourneyControllerInterface'])) {
        $this->tourneyFormatPlugin = new $tourneyFormatPlugin($tournament);
      }
      else {
        trigger_error(check_plain(t('Unable to create !tourneyFormatPlugin matches. Class does not implement TourneyControllerInterface.', array('!tourneyFormatPlugin' => $tourneyFormatPlugin))), E_USER_ERROR);
      }
    }
    $this->pluginInfo = self::getPluginInfo(get_class($this));
  }
  
  /**
   * Get the machine name of the plugin.
   */
  public static function getPluginInfo($controller) {
    $plugins = tourney_get_types();
    foreach ($plugins as $info) {
      if ($info['controller'] == $controller) {
        return $info;
      }
    }
  }
  
  /**
   * Create matches for a tournament.
   */
  public function createMatches() {
    // Create the actual match entities and the first game.
    $this->save();
  }

  /**
   * Build the match entities for the tournament. The array construction is
   * delegated to the plugin of this tournament.
   */
  public function build() {
    // Build the match structure
    $this->matches = $this->tourneyFormatPlugin->build();
  }

  /**
   * Wrapper function for saving matches. Delegate saving matches to the plugin.
   */
  public function save() {
    // Make sure we have a matches array to save.
    if (empty($this->matches)) {
      $this->build();
    }
    $this->tourneyFormatPlugin->saveMatches($this->tournament);
  }

  /**
   * Default method for saving matches. Plugins override this and define their
   * own way to save matches.
   * 
   * Loops through all the built match elements and creates new matches, along 
   * with their initial games and appropriate relations.
   * 
   * If a subclass changes the way matches are saved, the subclass will likely
   * need to change the getMatch() method. getMatch() and getMatchAddress() is 
   * dependent on saveMatches().
   * 
   * @see TourneyController::getMatch()
   *
   * @param $tournament
   *   The tournament object this match is related to.
   */
  protected function saveMatches($tournament) {
    $matches = $this->matches;
    foreach ( $matches as $mid => $match ) {
      // Set $id to the round number
      @list(, $bracket_id) = explode('-', $match['bracket_name']);
      @list(, $round_num) = explode('-', $match['round_name']);
      @list(, $match_num) = explode('-', $match['match_name']);
      $values = array(
        'type' => 'match',
        'title' => 'Match ' . $match_num,
        'name' => $tournament->name . '_' . $match['match_name'],
        'games' => variable_get('tourney_games_all', TOURNEY_GAMES),
      );
      $new_match = new TourneyMatch($values);
      $new_match->save();

      // Create the Tourney/Match relation and save bracket and round data to it
      $rid = tourney_create_relation('has_match', 'tourney_tournament', $tournament->id, 'tourney_match', $new_match->id);
      $relation = relation_load($rid);
      $relation->bracket[LANGUAGE_NONE][0]['value'] = $bracket_id;
      $relation->round[LANGUAGE_NONE][0]['value'] = $round_num;
      relation_save($relation);

      // Create the Match/Game relation
      $game = new TourneyGame(array(
        'type' => 'game',
        'title' => 'Game 1',
        'name' => $tournament->name . '_match-' . $new_match->id . '_game-1',
      ));
      $game->save();
      tourney_create_relation('has_game', 'tourney_match', $new_match->id, 'tourney_game', $game->id);
    }
  }
  
  /**
   * Return the match object based on a location in the bracket.
   * 
   * The SingleElimination plugin stores information about the location of the
   * match in a relation entity. This getMatch method is dependent on how the
   * saveMatches() method is written. If saveMatches() is overridden, it is
   * likely that getMatch() should be overridden as well.
   * 
   * @see TourneyController::saveMatches()
   * 
   * @param $round_num int
   *   The round number for the match.
   * @param $match_num int
   *   The match number for the match.
   * @return $match
   *   Returns a fully loaded match object.
   */
  public function getMatch($round_num, $match_num) {
    $query = relation_query('tourney_tournament', $this->tournament->id);
    $query->entityCondition('bundle', 'has_match')
          ->fieldCondition('round', 'value', $round_num);
    $results = $query->execute();
    
    $match_ids = array();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      $match_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
    }
    $matches = tourney_match_load_multiple($match_ids);
    
    foreach ($matches as $match) {
      if ($match->name == $this->tournament->name . '_match-'. $match_num) {
        return $match;
      }
    }
  }
  
  /**
   * Retrieve a match by machine name.
   * 
   * @todo: Get this working with EntityAPIControllerExportable.
   *   Loading entities by name comes with EntityAPIControllerExportable, but
   *   it EntityAPIControllerExportable removes rules integration, which we 
   *   need also. For now, do a db_query to find the entity id to load.
   * 
   * @param $args
   *   An array of variables passed from the plugin.
   * @return
   *   A full loaded match object. 
   */
  public function getMatchByName($args) {
    $entity_id = db_query("SELECT id FROM {tourney_match} WHERE name = :name",
      array(':name' => $this->tournament->name . '_' . $args['match_name']))
      ->fetchField();
      
    return tourney_match_load($entity_id);
  } 
  
  /**
   * Return the match location in the bracket of a match object.
   * 
   * The SingleElimination plugin stores information about the location of the
   * match in a relation entity. This getMatchAddress method is dependent on how
   * the saveMatches() method is written. If saveMatches() is overridden, it is
   * likely that getMatchAddress() should be overridden as well.
   * 
   * @see TourneyController::saveMatches()
   * 
   * @param $match
   *   The match object to get the address for
   * @return array
   *   An array keyed with 
   */
  protected function getMatchAddress($match) {
    $query = relation_query('tourney_match', $match->id);
    $query->entityCondition('bundle', 'has_match');
    $results = $query->execute();
    
    $name_parts = explode('_', $match->name);
    $match_ids = array();
    foreach ($results as $relation) {
      $r = relation_load($relation->rid);
      return array(
        'bracket' => $r->bracket[LANGUAGE_NONE][0]['safe_value'],
        'round_num' => $r->round[LANGUAGE_NONE][0]['safe_value'],
        'match_num' => substr(strstr($name_parts[1], '-'), 1),
      );
    }
  }
  /**
   * Gets a list of participating contestants in seed order
   *
   * @return array $contestants 
   *   List of contestants, all entities
   */
  public function getContestants() {
    if (is_null($this->tournament->contestants)) {
      // Take matches from only the first round, since those are the manually populated ones
      $brackets = array_shift($this->tournament->data);
      $matches = $brackets['rounds']['round-1']['matches'];

      // Set up two arrays to fill for seed order
      $seed_1 = array();
      $seed_2 = array();
      foreach ($matches as $match_callbacks) {
        $match = $this->tournament->tourneyFormatPlugin
          ->$match_callbacks['current_match']['callback']($match_callbacks['current_match']['args']);

        $seed = 1;
        foreach ( $match->getContestants() as $eid => $contestant ) {
          $group = "seed_" . $seed++;
          ${$group}[$eid] = $contestant;
        }
      }
      // Reverse seed_2, so we return contestants as 1 2 3 4 8 7 6 5 
      $this->tournament->contestants = array_merge($seed_1, array_reverse($seed_2));
    }
    
    return $this->tournament->contestants;
  }
  
  /**
   * Get the callbacks for this match from the structure of the plugin.
   * 
   * @param $match
   *   Match object.
   */
  public function getMatchCallbacks($match) {
    $match_location = $this->getMatchAddress($match);    
    // Return just the portion of the structure array that we need. We know how 
    // this structure array is built, because this array was defined in this
    // plugin.
    return $this->tournament->data['bracket-' . $match_location['bracket']]['rounds']['round-' . $match_location['round_num']]['matches']['match-' . $match_location['match_num']];
  }
  
  /**
   * Get round info for a match that is passed in. Traverses the structure of the 
   * tournament until it finds the match and returns information about the round
   * that contains the match
   * 
   * @param $match
   *   Match object
   * @param array
   *   An array with information about the round the match is in.
   */
  public function getRoundInfo($match) {
    @list(, $match_name) = explode('_', $match->name);
    return $this->findItem($match_name, $this->tournament->data);
  }
  
  /**
   * Get round info for a match that is passed in. Traverses the structure of the 
   * tournament until it finds the match and returns information about the round
   * that contains the match
   * 
   * @param $match
   *   Match object
   * @param array
   *   An array with information about the round the match is in.
   */
  public function getBracketInfo($match) {
    @list(, $match_name) = explode('_', $match->name);
    return $this->findItem($match_name, $this->tournament->data, array(), 4);
  }
  
  /**
   * Get the parent structure from structure array that contains the key element
   * that is being looked for.
   * 
   * @param $key_value
   *   The key to look for in the structure array.
   * @param $structure
   *   The structure array from the plugin.
   * @param $parent_structure
   *   The previous structure from the recursive function.
   * @param $depth
   *   The number of generations of parentage to return in the structure.
   * @return
   *   A portion of the structure array that contains the $key_value passed in.
   */
  protected function findItem($key_value, $structure, $parent_structures = array(), $depth = 1) {
    static $return = array();
    
    foreach ($structure as $key => $value) {
      if ($key == $key_value){
        $index = count($parent_structures) - $depth;
        $return = $parent_structures[$index];
        return;
      }
      elseif (is_array($value)) {
        $parent_structures[] = $structure;
        $this->findItem($key_value, $value, $parent_structures, $depth);
      }
    }
    return $return;
  }
}

/**
 * Default interface for any classes that will define a match style.
 */
interface TourneyControllerInterface {
  /**
   * Builds the array structure for all matches to be created.
   */
  public function build();
  
  /**
   * Defines the structure of matches in tournament.
   */
  public function structure();
  
  /**
   * Defines how to get the callbacks for a given match
   */
  public function getMatchCallbacks($match);
  
  /**
   * Determines which contestant is the winner.
   */
  public function determineWinner($tournament);
  
  /**
   * Determines if the tournament is finished.
   */
  public function isFinished($tournament);
}

abstract class TourneyEntity extends Entity {
  abstract public function isFinished();
  abstract public function isStarted();

  /**
   * Retrieve property. Property created by hook_entity_load().
   */
  public function getWinner() {
    return $this->winner;
  }
  public function delete() {
    watchdog('tourney', 'Deleted @type #@id.', array('@type' => $this->type, '@id' => $this->id));
    parent::delete();
  }
}
