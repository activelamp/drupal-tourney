<?php

/**
 * @file
 * A Double Elimination class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class DoubleElimination extends SingleElimination implements TourneyMatch {
  
  protected $num_bottom_rounds;

  protected function set_match_path($matches, $current_round, $current_match) {
    // Set the winner path in the previous round
    $previous_round = $current_round-1;
    if (array_key_exists('round-'. $previous_round, $matches)) {
      // Previous Match number.
      $pm = $current_match * 2;
      $previous_match = $pm - 1;
      
      // Set the winner path 
      $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-1';
      $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-2';
      // Set the loser path
      $matches['round-'. $previous_round]['match-'. $previous_match]['loser'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-1';
      $matches['round-'. $previous_round]['match-'. $pm]['loser'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-2';
      // Set the previous match for each contestant
      $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'-match-'. $previous_match;
      $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'round-'. $previous_round .'-match-'. $pm;
    }
    return $matches;
  }
  
  /**
   * Figure out how many rounds there should be based on $num_contestants,
   * then loop through and build all rounds.
   * 
   * @param $num_contestants
   *   The number of contestants in the first round of top bracket
   * @return $matches
   *   The matches array completely built out.
   */
  protected function build_rounds($num_contestants) {
    $top_bracket = parent::build_rounds($num_contestants);
    
    $bottom_bracket = $this->build_bottom_rounds($num_contestants);
    
    return array('top-bracket' => $top_bracket, 'bottom-bracket' => $bottom_bracket);
  }
  
  private function build_bottom_rounds($num_contestants) {
    // First round of play in losers bracket
    $loser_contestants = $num_contestants /= 2;
    $num_bottom_round = 1;
    $matches = array();
    
    // Build the first round
    $this->build_round($matches, $loser_contestants, $num_bottom_round);
    
    // Loop through each additonal round
    while ($num_contestants > 1) {
      // The winning losers that moved on
      $winning_losers = $loser_contestants / 2;
      $new_losers = $num_contestants /= 2;
      
      // Number of losers in this round.
      $loser_contestants = $winning_losers + $new_losers;
      
      if ($winning_losers == $new_losers) {
        // Increase round count.
        $num_bottom_round++;
        $this->build_round($matches, $loser_contestants, $num_bottom_round);
      }
      else {
        $num_bottom_round++;
        $this->build_round($matches, $winning_losers, $num_bottom_round);
        // This round did not involve anyone from the winners bracket. Set the
        // number of contestants to equal only the teams that played.
        $loser_contestants = $winning_losers;
        $num_contestants = $winning_losers;
      } 
    }
    $this->num_bottom_rounds = $num_bottom_round;
    return $matches;
  }
  
  private function build_round(&$matches, $contestants, $num_round) {
    for ($m=1;$m<=$contestants/2;$m++) {
      $matches['round-'. $num_round]['match-'. $m] = array(
        'contestant-1' => 'manual',
        'contestant-2' => 'manual',
      );
    }
  }
}