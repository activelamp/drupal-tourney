<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class RoundRobin implements TourneyMatchInterface {
  
  protected $num_contestants;
  protected $matches;
  
  /**
   * Constructor
   */
  public function __construct($num_contestants) {
    drupal_set_message('Round Robin');
    $this->num_contestants = $num_contestants;
  }
  
  /**
   * Build an array with all possible matches.
   * @see http://www.ehow.com/how_5796594_create-round_robin-schedule.html
   */
  public function build() {
    // Ensure we have an even number of spots
    $spots = $this->num_contestants % 2 ? $this->num_contestants + 1 : $this->num_contestants;
    // Number of rounds is (n - 1), n being contestants
    $rounds = $spots - 1;
    
    $matches = array();
    
    for ($r=1;$r<=$rounds;$r++) {
      for ($m=1;$m<=$spots/2;$m++) {
        $matches['round-'. $r]['match-'. $m]['contestant-1'] = $r == 1 ? 'manual' : '';
        $matches['round-'. $r]['match-'. $m]['contestant-2'] = $r == 1 ? 'manual' : '';
      }
    }
    dpm($matches);
    $this->matches = $matches;
    return $this->matches;
  }
}