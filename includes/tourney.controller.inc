<?php

/**
 * @file
 * Provides a controller to build matches after a tournament entity is created.
 */

// Maximum number of rounds that an elimination tournament will allow.
define('MAXIMUM_ROUNDS', 5);

class TourneyController implements TourneyControllerInterface {
  public $data;
  public $structure;
  public $numContestants;
  public $tournament;
  
  /**
   * Default theme implementations for every tournament format. We could have
   * defined these hooks directly in hook_theme(), but doing it this shows an
   * example of one might declare their own implementations in a plugin.
   * 
   * @see hook_theme().
   */
  public static function theme($existing, $type, $theme, $path) {
    $module_path = drupal_get_path('module', 'tourney');
    return array(
      'tourney_tournament_render' => array(
        'variables' => array('structure'),
        'path' => drupal_get_path('module', 'tourney') . '/theme',
        'template' => 'tourney-tournament-render',
        'file' => 'tourney.inc',
      ),
      'tourney_match_render' => array(
        'variables' => array('match'),
        'path' => $module_path . '/theme',
        'template' => 'tourney-match-render',
      ),
      'tourney_contestant' => array(
        'variables' => array('contestant', 'slot'),
        'path' => $module_path . '/theme',
        'template' => 'tourney-contestant',
      ),
    );
  }
  
  /**
   * Stub method to be defined in plugins.
   */
  public function preprocess($template, &$vars) {}

  /**
   * Round data generator
   *
   * @param array $data
   *   Uses 'id' from the array to set basic values, and joins for the rest
   *
   * @return $round
   *   Filled out round data array
   */
  public function buildRound($data) {
    $round = array(
      'title' => 'Round ' . $data['id'],
      'id'    => 'round-' . $data['id'],
    ) + $data;
    return $round;
  }

  /**
   * Match data generator
   *
   * @param array $data
   *   Uses 'id' from the array to set basic values, and joins for the rest
   *
   * @return $match
   *   Filled out match object
   */
  public function buildMatch($data) {
    $match = array(
      'controller'  => $this,
      'title'       => 'Match ' . $data['id'],
      'id'          => 'match-' . $data['id'],
      'match'       => $data['id'],
    ) + $data;
    return $data;
  }

  /**
   * Game data generator
   *
   * @param array $data
   *   Uses 'id' from the array to set basic values, and joins for the rest
   *
   * @return $match
   *   Filled out game object
   */
  public function buildGame($data) {
    $game = array(
      'title'       => 'Game ' . $data['game'],
      'id'          => 'game-' . $data['id'],
    ) + $data;
    return $game;
  }

  /**
   * Find elements given specific information
   *
   * @param string $data
   *   Data element from $this->data to search
   *
   * @param array $vars
   *   Keyed array of values on the elements to filter
   *   If one of the variables is an array, it will compare the testing
   *     element's value against each of the array's 
   *
   * @param boolean $first
   *   If TRUE, will return the first matched element
   *
   * @param string $specific
   *   Single value from each element to return, if not given will return
   *   the full element
   *
   * @return $elements
   *   Array of elements that match the $vars given
   */
  public function &find($data, $vars, $first = FALSE, $specific = NULL) {
    if ( !isset($this->data[$data]) ) return NULL;
    // Added in optimization to make routine find() calls faster
    // Normally searches are incremented, so this optimization holds
    // the place of the last call and continues the search from there
    //
    // Implementing this speeds calls from a 2048 contestant tournament
    //  up from 8.7 seconds to 2.1 seconds
    //
    // $optimize_data  : stores the last data array searched
    // $optimize_last  : the key left off on the last search
    // $optimize_until : in the case we return no elements in a search we
    //                   used optimization in, retry the search but only
    //                   until this key
    // $optimize_using : is set to determine whether we're optimizing
    //                   even after $optimize_last is cleared
    static $optimize_data  = NULL;
    static $optimize_last  = NULL;
    static $optimize_until = NULL;
           $optimize_using = $optimize_last;
    static $optimize_array = array();

    if ( $optimize_data !== $data ) {
      $optimize_last  = NULL;
      $optimize_until = NULL;
      $optimize_using = NULL;
      $optimize_data  = $data;
    }

    $elements = array();
    // is_array is expensive, set up an array to store this information
    $is_array = array();
    foreach ( $vars as $key => $value )
      $is_array[$key] = is_array($value);
    // Loop through all elements of the requested data array 
    foreach ( $this->data[$data] as $id => &$element ) {
      // We can only really optimize $first queries, since anything other
      // has to loop through all the elements anyways
      if ( $first && $optimize_last ) {
        // Until we hit the key we left off at, keep skipping elements...
        if ( $id !== $optimize_last ) continue;
        // ...and then we clear the variable so we can continue on.
        $optimize_last  = NULL;
      }
      // The other end of this is if we're continuing a failed optimized
      // search to exit out of the loop once we've hit where we started from
      if ( $optimize_until && $id == $optimize_until ) break;
      // Compare all our required $vars with its applicable properties
      // If that specific $vars is an array, check to see if the element's 
      // property is in the array
      // If the element fails at any of the checks, skip over it
      foreach ( $vars as $key => $value ) {
        if ( $element[$key] !== $value ) {
          if ( !$is_array[$key] || !in_array($element[$key], $value) ) 
            continue 2;
        }
      }
      // If we've supplied a 'specific' argument, only take that value,
      // otherwise take the entire element
      if ( $specific !== NULL )
        $elements[] = $data_is_array ? $element[$specific] : $element->$specific;
      else 
        $elements[] = &$element;
      // When $first, don't go any further once the first element has been set
      if ( $first === TRUE ) {
        // Set the optimizing static so we know where to start from next time
        $optimize_last = $id;
        return $elements[0];
      }
    } 
    // We're out of the loop, clear the static in case it went through all of 
    // the keys without stopping at one
    $optimize_last = NULL;
    // If we have no elements and we were using optimiziation...
    if ( !$elements && $optimize_using ) {
      // ...set the end key to what we started from
      $optimize_until = $optimize_using;
      $optimize_using = NULL;
      // and search again for
      $elements = $this->find($data, $vars, $first, $specific);
    } 
    return $elements;
  }
  
  /**
   * Default method for saving matches. Plugins can override this and define 
   * their own way to save matches. We're going to save matches as entities.
   * 
   * Iterates through the data array generated by plugin and creates new matches,
   * along with their initial games and appropriate relations.
   *
   * @param $tournament
   *   The tournament object this match is related to.
   */
  public function saveMatches() {
    $matches = $this->data['matches'];
    foreach ($matches as $mid => $match) {
      // Set $id to the round number
      $bracket_id = '';
      $round_num = '';
    
      // Set the games to zero for byes.
      if (array_key_exists('bye', $match) && $match['bye'] === TRUE) {
        continue;
      }
      $games = variable_get('tourney_games_all', TOURNEY_GAMES);
      $values = array(
        'type' => $this->tournament->get('match_bundle', 'match'),
        'label' => 'Match ' . $mid,
        'name' => $this->tournament->name . '_match-' . $mid,
        'games' => $games,
      );
      $new_match = new TourneyMatchEntity($values);
      $new_match->save();
    
      // Create the Tourney/Match relation and save bracket and round data to it
      $rid = tourney_create_relation('has_match', 'tourney_tournament', $this->tournament->id, 'tourney_match', $new_match->id);
      $relation = relation_load($rid);
      $relation->bracket[LANGUAGE_NONE][0]['value'] = $bracket_id;
      $relation->round[LANGUAGE_NONE][0]['value'] = $round_num;
      relation_save($relation);
    
      // Create the Match/Game relation
      if ($games > 0) {
        $game = new TourneyGameEntity(array(
          'type' => $this->tournament->get('game_bundle', 'game'),
          'title' => 'Game 1',
          'name' => $new_match->name . '_game-1',
        ));
        $game->save();
        tourney_create_relation('has_game', 'tourney_match', $new_match->id, 'tourney_game', $game->id);
      }
    }
  }
  
  /**
   * Retrieve a match by machine name.
   * 
   * @param $mid
   *   The match number to get
   * @return
   *   A full loaded match object. 
   */
  public function getMatchById($mid) {
    return $this->tournament->getMatchById($mid);
  }
  
  /**
   * Get a match uri for linking on a match block.
   * 
   * @param $mid
   *   The match number to get
   * @return
   *   The URI to the match as a string.
   */
  public function getMatchURI($mid) {
    return $this->tournament->getMatchURI($mid);
  }
  
  /**
   * Check to see if contestants can be set on match.
   * 
   * @param $mid
   *   The match number to check
   * @return
   *   Bitwise value with contestant slots turned on (slot 1 in right most position)
   */
  public function canSetContestants($mid) {
    return $this->tournament->canSetContestants($mid);
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
   * Plugin loader for the chosen tournament style.
   */
  public static function loadPlugin($tournament) {
    $tourneyFormatPlugin = $tournament->format;

    if (!class_exists($tourneyFormatPlugin)) {
      trigger_error(check_plain(t('Unable to load plugin. Class not found.', array('!tourneyFormatPlugin' => $tourneyFormatPlugin))), E_USER_ERROR);
    }

    $interfaces = class_implements($tourneyFormatPlugin);
    if (isset($interfaces['TourneyControllerInterface'])) {
      return new $tourneyFormatPlugin($tournament->players, $tournament);
    }
    else {
      trigger_error(check_plain(t('Unable to load plugin. Class does not implement TourneyControllerInterface.', array('!tourneyFormatPlugin' => $tourneyFormatPlugin))), E_USER_ERROR);
    }
  }
  
  /**
   * Get match info for a match that is passed in from plugin data.
   * 
   * @param $mid
   *   Match plugin id
   * @param array
   *   An array with information about the match
   */
  public function getMatchInfo($mid) {
    $this->build();
    return $this->data['matches'][$mid];
  }
  
  /**
   * Get the contestants that are related to this match
   * 
   * @param $mid
   *   Match plugin id
   */
  public function getMatchContestants($mid) {
    $match = $this->getMatchById($mid);
    // Ensure we have a match object, it's possible isn't created yet.
    return is_object($match) ? $match->getMatchContestants() : NULL;
  }
}

/**
 * Default interface for tourney format controllers.
 */
interface TourneyControllerInterface {
  /**
   * Builds the array structure for rounds.
   * 
   * @param $data
   *   The data array that is unionized of all tournament data.
   */
  public function buildRound($data);
  
  /**
   * Builds the array structure for matches to be saved.
   * 
   * @param $data
   *   The data array that is unionized of all tournament data.
   */
  public function buildMatch($data);
  
  /**
   * Builds the array structure for games.
   * 
   * @param $data
   *   The data array that is unionized of all tournament data.
   */
  public function buildGame($data);
}

/**
 * A Default factory for building matches and the first game of a match for a
 * tourney tournament.
 * 
 * @legacy This controller is being phased out
 */
class TourneyControllerLegacy {
  private static $controller = NULL;
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
   * @deprecated
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

      // Set the games to zero for byes.
      $games = array_key_exists('bye', $match['match_info']) && $match['match_info']['bye']
        ? 0 : variable_get('tourney_games_all', TOURNEY_GAMES);

      $values = array(
        'type' => $tournament->get('match_bundle', 'match'),
        'title' => 'Match ' . $match_num,
        'name' => $tournament->name . '_' . $match['match_name'],
        'games' => $games,
      );
      $new_match = new TourneyMatchEntity($values);
      $new_match->save();

      // Create the Tourney/Match relation and save bracket and round data to it
      $rid = tourney_create_relation('has_match', 'tourney_tournament', $tournament->id, 'tourney_match', $new_match->id);
      $relation = relation_load($rid);
      $relation->bracket[LANGUAGE_NONE][0]['value'] = $bracket_id;
      $relation->round[LANGUAGE_NONE][0]['value'] = $round_num;
      relation_save($relation);

      // Create the Match/Game relation
      if ($games > 0) {
        $game = new TourneyGameEntity(array(
          'type' => $tournament->get('game_bundle', 'game'),
          'title' => 'Game 1',
          'name' => $new_match->name . '_game-1',
        ));
        $game->save();
        tourney_create_relation('has_game', 'tourney_match', $new_match->id, 'tourney_game', $game->id);
      }
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
      if ( $match->name == $this->tournament->name . '_match-' . $match_num ) {
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
   * @deprecated
   */
  public function getContestants() {
    if (is_null($this->tournament->contestants)) {
      // Take matches from only the first round, since those are the manually populated ones
      $copy = $this->tournament->data;
      $brackets = array_shift($copy);
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
   * Get match info for a match that is passed in. Traverses the structure of the 
   * tournament until it finds the match and returns information about the match
   * 
   * @param $match
   *   Match object
   * @param array
   *   An array with information about the match
   */
  public function getMatchInfo($match) {
    @list(, $match_name) = explode('_', $match->name);
    return $this->findMatchInfo($match_name, $this->tournament->tourneyFormatPlugin->structure());
  }
  
  /**
   * Get the parent structure from structure array that contains the key element
   * that is being looked for.
   * 
   * @param $key_value
   *   The key to look for in the structure array.
   * @param $structure
   *   The structure array from the plugin.
   * @return
   *   A portion of the structure array that contains the $key_value passed in.
   */
  protected function findMatchInfo($key_value, $structure) {
    // Rewritten, since we know the structure of the brackets
    foreach ( $structure as $b_name => $bracket ) {
      foreach ( $bracket['rounds'] as $r_name => $round ) {
        if ( !array_key_exists('matches', $round) ) continue;
        foreach ( $round['matches'] as $m_name => $match ) {
          if ( $m_name == $key_value ) return $match;
        }
      }
    }
  }
}

/**
 * Default interface for any classes that will define a match style.
 */
interface TourneyControllerLegacyInterface {
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
