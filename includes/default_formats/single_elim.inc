<?php

/**
 * @file
 * A Single Elimination class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class SingleElimination implements TourneyMatch {
  
  protected $num_contestants;
  protected $num_rounds;
  protected $matches;
  
  /**
   * Constructor
   */
  public function __construct($num_contestants) {
    $this->calculate_rounds($num_contestants);
    $this->set_message();
  }
  
  /**
   * Build the nested matches array with all possible matches.
   * 
   * @return $matches
   *   The matches array completely built out.
   */
  public function build() {
    $matches = array();
    
    // Build all rounds
    $this->matches = $this->build_rounds($this->num_contestants);
    
    return $this->matches;
  }
  
  /**
   * Figure out how many rounds there should be based on $contestants_in_round,
   * then loop through and build all rounds.
   * 
   * @param $contestants_in_round
   *   The number of contestants in the first round.
   * @return $matches
   *   The matches array completely built out.
   */
  protected function build_rounds($contestants_in_round) {
    $matches = array();
    $this->calculate_rounds($contestants_in_round);
    $num_rounds = $this->num_rounds;

    for ($r=1;$r<=$num_rounds;$r++) {
      $matches['round-'. $r] = array();
      // Loop through all matches for this round
      for ($m=1;$m<=$contestants_in_round/2;$m++) {
        $matches['round-'. $r]['match-'. $m] = array(
          'contestant-1' => 'manual',
          'contestant-2' => 'manual',
        );
        
        $matches = $this->set_match_path($matches, $r, $m);
      }
      $contestants_in_round /= 2;
    }
    return $matches;
  }
  
  protected function set_match_path($matches, $current_round, $current_match) {
    // Set the winner path in the previous round
    $previous_round = $current_round-1;
    if (array_key_exists('round-'. $previous_round, $matches)) {
      // Previous Match number.
      $pm = $current_match * 2;
      $previous_match = $pm - 1;
      $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-1';
      $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-2';
      $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'-match-'. $previous_match;
      $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'round-'. $previous_round .'-match-'. $pm;
    }
    return $matches;
  }
  
  /**
   * Getter for the number of contestants
   */
  protected function get_num_contestants() {
    return $this->num_contestants;
  }
  
  protected function set_message() {
    drupal_set_message(get_class($this) .' with '. $this->num_contestants .' contestants and '. $this->num_rounds .' rounds.');
  }
    
  private function calculate_rounds($num_contestants) {
    // Make sure we don't have too many contestants.
    $maximum_contestants = pow(2, MAXIMUM_ROUNDS);
    if ($num_contestants > $maximum_contestants) {
      drupal_set_message(t('Tournaments can only be !num rounds at the most with !player contestants. Some teams will not be able to play.', array('!num' => MAXIMUM_ROUNDS, '!player' => $maximum_contestants)), 'warning');
      $num_contestants = $maximum_contestants;
    }
    
    // Store values of exponents with base 2
    $possible_count = array();

    // Figure out how many rounds we're going to have. 
    for ($i=1;$i<=MAXIMUM_ROUNDS;$i++) {
      if ($num_contestants == pow(2, $i)) {
        $this->num_rounds = $i;
        $this->num_contestants = $num_contestants;
        return;
      }
      $possible_count[] = pow(2, $i);
    }
    // Unable to calculate rounds because the number of contestants passed in.
    $this->fix_contestant_count($num_contestants, $possible_count) ;
    
  }
  
  private function fix_contestant_count($num_contestants, $possible_count) {
    // If the number of contestants is not a base two exponent. Figure out how
    // many contestants we need to bump up to.
    for ($i=0;$i<=count($possible_count);$i++) {
      // Find between what valid possible counts the number of contestants is in.
      if ($num_contestants > $possible_count[$i] && $num_contestants < $possible_count[$i+1]) {
        // Set the number of contestants to the higher number
        $new_num_contestants = $possible_count[$i+1];
        
        // Calculate number of rounds now that we fixed contestant count.
        $this->calculate_rounds($new_num_contestants);
        return;
      }
    }
  }
}