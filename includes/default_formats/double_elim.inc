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
  // Define a status variable set for building bottom bracket.
  private $build_bottom = false; 
  // Define a status variable for match between losers. @see set_match_path().
  private $losers_only = false; 
  private $loser_slot = 1;
  
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
    // Set a flag for building bottom bracket.
    $this->build_bottom = true;
    $bottom_bracket = $this->build_bottom_rounds($num_contestants);
    
    // The Champion bracket is the last round when both brackets converge. It
    // always looks the same.  Defining the bracket explicityly.
    $champion_match = $first_loss = array();
    $this->build_round($champion_match, 2, 1);
    $this->build_round($first_loss, 2, 2);
    
    return array(
      'top-bracket' => $top_bracket, 
      'bottom-bracket' => $bottom_bracket,
      'champion-bracket' => array_merge($champion_match, $first_loss),
    );
  }
  
  protected function set_match_path($matches, $current_round, $current_match) {
    $previous_round = $current_round-1;
    if (array_key_exists('round-'. $previous_round, $matches)) {
      // Previous Match number.
      $pm = $current_match * 2;
      $previous_match = $pm - 1;
      if (!$this->build_bottom) {
        $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-1';
        $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'round-'. $current_round .'-match-'. $current_match . '-contestant-2';
        $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'-match-'. $previous_match;
        $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'round-'. $previous_round .'-match-'. $pm;
      }
      // Building the bottom bracket logic
      else {
        if (array_key_exists('match-'. $previous_match, $matches['round-'. $previous_round])) {
          $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'round-'. $current_round .'-matchaa-'. $current_match . '-contestant-1';
          $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'-matchaa-'. $previous_match;
          $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'from-winners-bracket';
        }
        if (array_key_exists('match-'. $pm, $matches['round-'. $previous_round])) {
          if (!$this->losers_only) {
            $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'round-'. $current_round .'-matchbb-'. $current_match . '-contestant-1';
            $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'-match-bb'. $previous_match;
            $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'from-winners-bracket';
          }
          else {
            $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'round-'. $current_round .'-matchcc-'. ++$current_match . '-contestant-2';
            $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'-match-cc'. $previous_match;
            $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'round-'. $previous_round .'-match-dd'. $pm;
          }
        }
      }
    }
    
    return $matches;
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
        $this->losers_only = true;
        $this->build_round($matches, $winning_losers, $num_bottom_round);
        // This round did not involve anyone from the winners bracket. Set the
        // number of contestants to equal only the teams that played.
        $loser_contestants = $winning_losers;
        $num_contestants = $winning_losers;
        $this->losers_only = false;
      } 
    }
    $this->num_bottom_rounds = $num_bottom_round;
    return $matches;
  }
}