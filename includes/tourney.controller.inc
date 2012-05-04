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
class TourneyFactory {

  protected $num_contestants;
  protected $match_style;
  protected $matches;
  protected $tournament;
  protected $match_path = array();
  protected $match_count = 0;

  /**
   * Constructor
   */
  public function __construct($num_contestants, $match_style) {
    $this->num_contestants = $num_contestants;
    if (!class_exists($match_style)) {
      trigger_error(check_plain(t('Unable to create !match_style matches. Class not found.', array('!match_style' => $match_style))), E_USER_ERROR);
    }
    
    $interfaces = class_implements($match_style);
    if (isset($interfaces['TourneyFactoryInterface'])) {
      $this->match_style = new $match_style($num_contestants);
    }
    else {
      trigger_error(check_plain(t('Unable to create !match_style matches. Class does not implement TourneyFactoryInterface.', array('!match_style' => $match_style))), E_USER_ERROR);
    }
  }

  /**
   * Create the match entities for the tournament.
   */
  public function create() {
    // Build the match structure
    $this->matches = $this->match_style->build();
  }

  /**
   * Saves the matches for the tournament.
   *
   * @param $tournament
   *   The entity object of the tournament the matches are being saved in.
   * @return $match_count
   *   Returns the number of matches saved by this tournament.
   */
  public function save($tournament) {
    $this->tournament = $tournament;
    $this->saveMatches($this->matches);
    return $this->match_count;
  }

  private function saveMatches($matches) {
    foreach ( $matches as $mid => $match ) {
      krumo($match);
      $values = array(
        'type' => 'match',
        'title' => 'match-' . $match['match'],
        'games' => variable_get('tourney_games_all', TOURNEY_GAMES),
      );
      $new_match = new TourneyMatch($values);
      $new_match->save();

      $rid = tourney_create_relation('has_match', 'tourney_tournament', $this->tournament->id, 'tourney_match', $new_match->id);
      $relation = relation_load($rid);
      $relation->bracket[LANGUAGE_NONE][0]['value'] = $match['bracket'];
      $relation->round[LANGUAGE_NONE][0]['value'] = $match['round'];
      relation_save($relation);

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
interface TourneyFactoryInterface {
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