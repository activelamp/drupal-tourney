<?php

/**
 * @file
 * A Ladder class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class LadderController extends TourneyController implements TourneyControllerInterface {

  protected $num_contestants;
  protected $matches;

  /**
   * Constructor
   */
  public function __construct(TourneyTournament $tournament) {
    drupal_set_message(t('Ladder match not implemented yet.'), 'warning');
    $this->num_contestants = $tournament->num_players;
    
    parent::__construct();
  }

  /**
   * Build an array with all possible matches.
   */
  public function build() {
    
  }
  
  /**
   * Stub coe
   */
  public function structure() {
    return array();
  }
  
  /**
   * Stub code.
   */
  public function isFinished($tournament) {
    return false;
  }

  /**
   *
   */
  public function determineWinner($tournament) {
    $ranks = $tournament->fetchRanks();

    drupal_set_message(check_plain(t('Tournament !format not implemented yet!.', array('!format' => $tournament->format)), 'status'));

    return $this;
  }

}