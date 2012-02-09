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
   */
  public function build() {
    $matches = array();
    $i = 0;
    // Loop through all contestants
    for ($c=1;$c<$this->num_contestants;$c++) {
      $next_c = $c + 1;
      
      /**
       * Build Match for each contestant
       * 
       * Starting with the current contestant in this iteration of the current
       * loop, create a match with all contestants after this one.
       */
      $next_contestant = $c + 1;
      while ($next_contestant <= $this->num_contestants) {
        $matches['match-'. ++$i] = array(
          'contestant-'. $c => 'manual',
          'contestant-'. $next_contestant++ => 'manual',
        );
      }
    }
    $this->matches = $matches;
    return $this->matches;
  }
}