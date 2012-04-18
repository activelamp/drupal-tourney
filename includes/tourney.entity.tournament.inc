<?php

/**
 * @file
 * Tournament class.
 */

class TourneyTournament extends Entity {

  private $tournamentMatchIds = array();
  private $tournamentStarted  = NULL;
  private $tournamentFinished = NULL;
  private $tournamentWinner = NULL;
  public $tournamentFormatPlugin = NULL;

  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    $this->setup();
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
   * Expose the Tourney Format Builder plugin to the object.
   *
   * @return TourneyTournament
   *   TourneyTournament object for chaining.
   */
  protected function setup() {
    // No setup for newly created entities that haven't been saved yet.
    if (empty($this->id)) {
      return;
    }
    
    // Make sure we have a valid tourney format plugin for this tournament and
    // then load a builder object on the tourney object.
    if (!class_exists($this->format)) {
      throw new Exception(t('Tourney format plugin does not exist'));
    }   
    $this->tournamentFormatPlugin = new $this->format($this->players);
  }

  /**
   * Lazy load all the match ids for this tournament. Once the matches have 
   * been loaded for the tournament, it will return the 'cached' copy.
   */
  public function getMatchIds() {
    if (empty($this->tournamentMatchIds)) {
      $this->tournamentMatchIds = TourneyTournament::fetchMatchIds($this->id); 
    }
    return $this->tournamentMatchIds;
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
   * Determine if a tournament is finished (when all matches are finished).
   *
   * @return bool
   *   Return TRUE if the tournament is finished.
   */
  public function isFinished() {
    if (is_null($this->tournamentFinished)) {
      $this->tournamentFinished = $this->tournamentFormatPlugin->is_finished($this);
    }

    if (TOURNEY_DEBUG_EXTENDS) {
      $output = ($this->tournamentFinished) ? 'FINISHED!' : 'IN PROGRESS...';
      dpr('TOURNAMENT ' . $this->id . ': ' . $output);
    }

    return $this->tournamentFinished;
  }

  /**
   * Set the winner property to the winner of this match.
   */
  public function determineWinner() {
    if ($this->isFinished()) {
      $this->tournamentFormatPlugin->determine_winner($this);
    }

    return $this;
  }

}