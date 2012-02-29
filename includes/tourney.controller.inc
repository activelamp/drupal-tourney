<?php

/**
 * @file
 * Provides a controller to build matches after a tournament entity is created.
 */

// Maximum number of rounds that an elimination tournament will allow.
define('MAXIMUM_ROUNDS', 5);
 
/**
 * A Default controller for building matches.
 */
class TourneyBuildMatches {
  
  protected $num_contestants;
  protected $match_style;
  protected $matches;
  protected $tournament;
  protected $match_path = array();
  
  /**
   * Constructor
   */
  public function __construct($num_contestants, $match_style) {
    $this->num_contestants = $num_contestants;
    if (!class_exists($match_style)) {
      trigger_error(t('Unable to create !match_style matches. Class not found.', array('!match_style' => $match_style)), E_USER_ERROR);
    }
    $interfaces = class_implements($match_style);
    if (isset($interfaces['TourneyMatchInterface'])) {
      $this->match_style = new $match_style($num_contestants);
    }
    else {
      trigger_error(t('Unable to create !match_style matches. Class does not implement TourneyMatchInterface.', array('!match_style' => $match_style)), E_USER_ERROR);
    }
  }
  
  /**
   * Create the match entities for the tournament.
   */
  public function create() {
    drupal_set_message('The matches were created');

    // Build the match structure
    $this->matches = $this->match_style->build();
  }
  
  /**
   * Saves the matches for the tournament.
   * 
   * @param $tournament
   *   The entity object of the tournament the matches are being saved in.
   */
  public function save($tournament) {
    $this->tournament = $tournament;
    $this->save_match($this->matches);
  }
  
  private function save_match($matches) {
    static $round;
    static $bracket;
    dpm($matches);
    $identifiers = array('bracket', 'round');
    foreach ($matches as $key => $value) {
      foreach ($identifiers as $identifier) {
        // We only want matches, but keep bracket and round metadata.
        if (strpos($key, $identifier .'-') === 0 && is_array($value)) {
          ${$identifier} = $key;
        }
      }
      if (strpos($key, 'match-') !== 0 && is_array($value)) {
       $this->save_match($value);
      }
      else {
        // Build the match object to save
        $match = new stdClass();
        $match->type = 'match';
        $match->title = $key;
        $match->games = 5;
        $match->next_match_winner = array_key_exists('winner', $value) ? $value['winner'] : '';
        $match->next_match_loser = array_key_exists('loser', $value) ? $value['loser'] : '';
        $match->previous_match1 = array_key_exists('previous-1', $value) ? $value['previous-1'] : 'manual';
        $match->previous_match2 = array_key_exists('previous-2', $value) ? $value['previous-2'] : 'manual';
        tourney_match_save($match);
        
        // Add the relationship to the tournament.
        $rid = tourney_create_relation('has_match', 'tourney_tournament', $this->tournament->id, 'tourney_match', $match->id);
        $relation = relation_load($rid);
        $relation->bracket[LANGUAGE_NONE][0]['value'] = $bracket;
        $relation->round[LANGUAGE_NONE][0]['value'] = $round;
        relation_save($relation);
        
        // Create the first game of the match
        $game = new stdClass();
        $game->type = 'game';
        $game->title = 'game-1';
        tourney_game_save($game);
        
        // Add the relationship to the game.
        $rid = tourney_create_relation('has_game', 'tourney_match', $match->id, 'tourney_game', $game->id);
      }
    }
  }
}

/**
 * Default interface for any classes that will define a match style.
 */
interface TourneyMatchInterface {
  /**
   * Builds the array structure for all matches to be created.
   */
  public function build();
}
