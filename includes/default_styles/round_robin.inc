<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class RoundRobin implements TourneyMatch {
  
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
    
    // Loop through all contestants
    for ($c=1;$c<$this->num_contestants;$c++) {
      $i = 0;
      $next_c = $c + 1;
      
      // Build Match for each contestant
      while ($i != ($this->num_contestants - 1)) {
        $matches['match-'. ++$i] = array(
          'contestant-'. $c => 'manual',
          'contestant-'. $next_c => 'manual',
        );
      }
      
    }
    
    $this->matches = $matches;
    dpm($matches);
    return $this->matches;
  }
}