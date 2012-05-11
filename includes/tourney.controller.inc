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
  protected $matchStyle;
  protected $matches;
  protected $tournament;

  /**
   * Constructor
   */
  public function __construct($tournament) {
    $this->tournament = $tournament;
    $matchStyle = $tournament->format;
    
    if (!class_exists($matchStyle)) {
      trigger_error(check_plain(t('Unable to create !matchStyle matches. Class not found.', array('!matchStyle' => $matchStyle))), E_USER_ERROR);
    }
    
    $interfaces = class_implements($matchStyle);
    if (isset($interfaces['TourneyControllerInterface'])) {
      $this->matchStyle = new $matchStyle($tournament->players);
    }
    else {
      trigger_error(check_plain(t('Unable to create !matchStyle matches. Class does not implement TourneyControllerInterface.', array('!matchStyle' => $matchStyle))), E_USER_ERROR);
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
    $this->matches = $this->matchStyle->build();
  }

  /**
   * Wrapper function for saving matches. Delegate saving matches to the plugin.
   */
  public function save() {
    // Make sure we have a matches array to save.
    if (empty($this->matches)) {
      $this->build();
    }
    $this->matchStyle->saveMatches($this->tournament->id);
  }

  /**
   * Default method for saving matches. Plugins override this and define their
   * own way to save matches.
   * 
   * Loops through all the built match elements and creates new matches, along 
   * with their initial games and appropriate relations.
   *
   * @param $tournament_id
   *   The id of the tournament this match is related to.
   */
  protected function saveMatches($tournament_id) {
    $matches = $this->matches;
    foreach ( $matches as $mid => $match ) {
      $values = array(
        'type' => 'match',
        'title' => 'match-' . $match['match'],
        'games' => variable_get('tourney_games_all', TOURNEY_GAMES),
      );
      $new_match = new TourneyMatch($values);
      $new_match->save();

      // Create the Tourney/Match relation and save bracket and round data to it
      $rid = tourney_create_relation('has_match', 'tourney_tournament', $tournament_id, 'tourney_match', $new_match->id);
      $relation = relation_load($rid);
      $relation->bracket[LANGUAGE_NONE][0]['value'] = $match['bracket'];
      $relation->round[LANGUAGE_NONE][0]['value'] = $match['round'];
      relation_save($relation);

      // Create the Match/Game relation
      $game = new TourneyGame(array(
        'type' => 'game',
        'title' => 'game-1',
      ));
      $game->save();
      tourney_create_relation('has_game', 'tourney_match', $new_match->id, 'tourney_game', $game->id);
    }
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