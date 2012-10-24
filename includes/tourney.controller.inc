<?php

/**
 * @file
 * Provides a controller to build matches after a tournament entity is created.
 */

// Maximum number of rounds that an elimination tournament will allow.
define('MAXIMUM_ROUNDS', 5);

/**
 * Tournament plugins are responsible for four things:
 *
 *   1. Define the structure data of a tournament.
 *   2. Define the match data in a tournament
 *   3. Provide data (and logic) of what the next and previous match is for both
 *      winners and losers of a match.
 *   4. How to render the tournament.
 *
 * Beyond the four items listed above it is up to the system implementing the
 * plugin to decide what to do with the data the plugin provides.
 *
 * Plugins should extend TourneyController to integrate with the Drupal entity
 * system that saves matches and the first game of each match. TourneyController
 * is the gateway between the plugin (the data and logic of tournament) and the
 * Drupal entity system.
 *
 * The key component to every plugin is how the plugin builds its data array.
 * TourneyController provides helper methods to create arrays that hold important
 * data that is present in every tournament. Every tournament has some concept
 * of a bracket, round, match, and game. Plugin build() methods are encouraged
 * to use the build methods in TourneyController for these arrays:
 *
 * @code
 * TourneyController::buildBracket($data);
 * TourneyController::buildRound($data);
 * TourneyController::buildMatch($data);
 * TourneyController::buildGame($data);
 * @endcode
 *
 * The most important data structure your plugin should implement in build() is
 * TourneyController::buildMatch(). It is from this array that matches are saved
 * to the Drupal entity system using TourneyController::saveMatches().
 *
 * Plugins can render on their own with no knowledge of a tournament entity. If
 * a plugin was instantiated with a tournament entity, then information from the
 * tournament will be preprocessed into the plugin render, and links to matches,
 * and contestant names will appear.
 *
 * However, plugins should not be dependent on a tournament object being present.
 * As an example, the following code will instantiate a new plugin with
 * eight contestants, set a plugin option for a third place match, and then
 * render the tournament.
 *
 * @code
 * $tournament = new SingleEliminationController(8);
 * $tournament->setPluginOptions(array('third_place' => TRUE));
 *
 * $tournament->render();
 * @endcode
 *
 * This code will create all the matches in a data array, and render a tournament
 * bracket. If the plugin where loaded with a valid TourneyTournamentEntity, it
 * will render the same bracket with information from the tournament in the
 * bracket.
 */
class TourneyController implements TourneyControllerInterface {
  public $data = NULL;
  public $structure;
  public $numContestants;
  public $tournament;
  public $pluginOptions = array();
  public $moveWinners = TRUE;

  /**
   * Constructor
   */
  public function __construct() {
    $this->pluginInfo = self::getPluginInfo(get_class($this));
  }

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
        'variables' => array('plugin' => NULL),
        'path' => drupal_get_path('module', 'tourney') . '/theme',
        'template' => 'tourney-tournament-render',
        'file' => 'tourney.inc',
      ),
      'tourney_match_render' => array(
        'variables' => array('match' => NULL, 'plugin' => NULL),
        'path' => $module_path . '/theme',
        'template' => 'tourney-match-render',
        'file' => 'tourney.inc',
      ),
      'tourney_contestant' => array(
        'variables' => array('contestant', 'slot'),
        'path' => $module_path . '/theme',
        'template' => 'tourney-contestant',
      ),
    );
  }

  /**
   * Reset static variables anytime data array is built.
   */
  public function build() {
    drupal_static_reset('match');
    drupal_static_reset('round');
  }

  /**
   * Methods that must be defined in plugin subclass.
   *
   * @todo: Should these be abstract? Do we want to provide a default
   *   implemenations?
   */
  public function populatePositions() {}
  public function populateSeedPositions() {}
  public function preprocess($template, &$vars) {}
  public function render() {}

  /**
   * Bracket data generator
   *
   * @param array $data
   *   Uses 'id' from the array to set basic values, and joins for the rest
   *
   * @return $round
   *   Filled out round data array
   */
  public function buildBracket($data) {
    $round = array(
      'title' => ucfirst($data['id']) . ' Bracket',
      'id'    => 'bracket-' . $data['id'],
    ) + $data;
    return $round;
  }

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
    if (!isset($this->data[$data])) {
      return NULL;
    }
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

    if ($optimize_data !== $data) {
      $optimize_last  = NULL;
      $optimize_until = NULL;
      $optimize_using = NULL;
      $optimize_data  = $data;
    }

    $elements = array();
    // is_array is expensive, set up an array to store this information
    $is_array = array();
    foreach ($vars as $key => $value)
      $is_array[$key] = is_array($value);
    // Loop through all elements of the requested data array
    foreach ($this->data[$data] as $id => &$element) {
      // We can only really optimize $first queries, since anything other
      // has to loop through all the elements anyways
      if ($first && $optimize_last) {
        // Until we hit the key we left off at, keep skipping elements...
        if ($id !== $optimize_last) {
          continue;
        }
        // ...and then we clear the variable so we can continue on.
        $optimize_last  = NULL;
      }
      // The other end of this is if we're continuing a failed optimized
      // search to exit out of the loop once we've hit where we started from
      if ($optimize_until && $id == $optimize_until) {
        break;
      }
      // Compare all our required $vars with its applicable properties
      // If that specific $vars is an array, check to see if the element's
      // property is in the array
      // If the element fails at any of the checks, skip over it
      foreach ($vars as $key => $value) {
        if (!isset($element[$key]) || $element[$key] !== $value) {
          if (!$is_array[$key] || !in_array($element[$key], $value))
            continue 2;
        }
      }
      // If we've supplied a 'specific' argument, only take that value,
      // otherwise take the entire element
      if ($specific !== NULL)
        $elements[] = $data_is_array ? $element[$specific] : $element->$specific;
      else
        $elements[] = &$element;
      // When $first, don't go any further once the first element has been set
      if ($first === TRUE) {
        // Set the optimizing static so we know where to start from next time
        $optimize_last = $id;
        $optimize_until = NULL;
        return $elements[0];
      }
    }
    // We're out of the loop, clear the static in case it went through all of
    // the keys without stopping at one
    $optimize_last  = NULL;
    $optimize_until = NULL;
    // If we have no elements and we were using optimiziation...
    if (!$elements && $optimize_using) {
      // ...set the end key to what we started from
      $optimize_until = $optimize_using;
      $optimize_using = NULL;
      // and search again for
      $elements = &$this->find($data, $vars, $first, $specific);
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
    $create_alias = FALSE;
    if (module_exists('pathauto')) {
      module_load_include('inc', 'pathauto');
      $create_alias = TRUE;
    }

    foreach ($matches as $mid => $match) {
      // Set $id to the round number
      $bracket_id = '';
      $round_num = '';

      // Set the games to zero for byes.
      if (array_key_exists('bye', $match) && $match['bye'] === TRUE) {
        continue;
      }
      $games = $this->tournament->get('default_games', TOURNEY_GAMES);
      $values = array(
        'type' => $this->tournament->get('match_bundle', 'match'),
        'label' => 'Match ' . $mid,
        'name' => $this->tournament->name . '_match-' . $mid,
        'games' => $games,
      );
      $new_match = new TourneyMatchEntity($values);
      $new_match->save();
      if ($create_alias && pathauto_pattern_load_by_entity($new_match->entity_type, $new_match->type)) {
        $uri = entity_uri($new_match->entity_type, $new_match);
        pathauto_create_alias('tourney', 'insert', $uri['path'], array($new_match->entity_type => $new_match), $new_match->type);
      }


      // Create the Tourney/Match relation and save bracket and round data to it
      $rid = tourney_create_relation('has_match', 'tourney_tournament', $this->tournament->id, 'tourney_match', $new_match->id);

      // Create the Match/Game relation
      if ($games > 0) {
        $game = new TourneyGameEntity(array(
          'type' => $this->tournament->get('game_bundle', 'game'),
          'title' => 'Game 1',
          'name' => $new_match->name . '_game-1',
        ));
        $game->save();
        if ($create_alias && pathauto_pattern_load_by_entity($game->entity_type, $game->type)) {
          $uri = entity_uri($game->entity_type, $game);
          pathauto_create_alias('tourney', 'insert', $uri['path'], array($game->entity_type => $game), $game->type);
        }
        tourney_create_relation('has_game', 'tourney_match', $new_match->id, 'tourney_game', $game->id);
      }
    }
  }

  /**
   * Retrieve a match by machine name if we have a tournament.
   *
   * @param $mid
   *   The match number to get
   * @return
   *   A full loaded match object.
   */
  public function getMatchById($mid) {
    if (is_object($this->tournament)) {
      return $this->tournament->getMatchById($mid);
    }
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
   * Set the plugin options for this tourney format.
   *
   * @param array $values
   *   Associative array keyed by plugin class name:
   *   - PluginClassName: (array) Associative array of plugin options keyed by
   *     option name.
   */
  public function setPluginOptions($values) {
    foreach ($values as $key => $value) {
      $this->pluginOptions[$key] = $value;
    }
  }

  /**
   * Get the plugin options for this tourney format.
   */
  public function getPluginOptions() {
    // Get plugin options from a tournament
    $this->tourneyPluginOptions = is_object($this->tournament)
      ? $this->tournament->get("**ALL**") : array();

    // Allow tournament plugin options to override code options.
    return $this->pluginOptions = array_merge($this->pluginOptions, $this->tourneyPluginOptions);
  }

  /**
   * Get the machine name of the plugin.
   *
   * @param $controller_name
   *   The name of the controller to find plugin information for.
   */
  public static function getPluginInfo($controller_name) {
    $plugins = tourney_get_types();
    foreach ($plugins as $info) {
      if ($info['controller'] == $controller_name) {
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
    // Only build the data array if it is not available.
    if (is_null($this->data)) {
      $this->build();
    }
    return array_key_exists($mid, $this->data['matches']) ? $this->data['matches'][$mid] : NULL;
  }
}

/**
 * Default interface for tourney format controllers.
 *
 * @todo: Update this interface with all methods that the tourney module expects
 *   to call on the plugin for.
 */
interface TourneyControllerInterface {
  /**
   * Plugin build() methods are encouraged to use the build methods in
   * TourneyController for these arrays:
   *
   * @code
   * TourneyController::buildBracket($data);
   * TourneyController::buildRound($data);
   * TourneyController::buildMatch($data);
   * TourneyController::buildGame($data);
   * @endcode
   *
   * The most important data structure your plugin should implement in build() is
   * TourneyController::buildMatch(). It is from this array that matches are saved
   * to the Drupal entity system using TourneyController::saveMatches()
   */
  public function build();

  /**
   * Populate next/previous match pathing on the matches data array for each match.
   */
  public function populatePositions();

  /**
   * Calculate and fill seed data into matches. Also marks matches as byes if
   * the match is a bye.
   */
  public function populateSeedPositions();

  /**
   * Render the tournament
   */
  public function render();
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
