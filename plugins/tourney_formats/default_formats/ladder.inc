<?php

/**
 * @file
 * A Ladder class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class Ladder implements TourneyMatchInterface {
  
  protected $num_contestants;
  protected $matches;
  
  /**
   * Constructor
   */
  public function __construct($num_contestants) {
    drupal_set_message('Ladder match not implemented yet.');
    $this->num_contestants = $num_contestants;
  }
  
  /**
   * Build an array with all possible matches.
   */
  public function build() {
    
  }

  /**
   *
   */
  public function determine_winner($tournament) {
    $ranks = $tournament->fetchRanks();

    drupal_set_message(t('Tournament ' . $tournament->format. ' not implemented yet!.'), 'status');

    return $this;
  }

}