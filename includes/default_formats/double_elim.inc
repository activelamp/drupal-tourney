<?php

/**
 * @file
 * A Double Elimination class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class DoubleElimination extends SingleElimination implements TourneyMatchInterface {
  
  protected $num_bottom_rounds;
  // Set a variable when contestants in round are from the same bracket.
  private $same_bracket = true;
  // Track round numbers that new contestants not from the same bracket enter
  // the round.
  private $bracket_alignment = array();
  
  
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
    $this->same_bracket = false;
    $bottom_bracket = $this->build_bottom_rounds($num_contestants);
    // Fix the loser path in top bracket, now that the bottom bracket has been
    // built.
    $this->fix_top_loser_path($top_bracket);
    
    // The Champion bracket is the last round when both brackets converge. It
    // always looks the same.  Defining the bracket explicityly.
    $champion_match = $first_loss = array();
    $this->build_round($champion_match, 2, 1);
    $this->build_round($first_loss, 2, 2);
    
    return array(
      'bracket-top' => $top_bracket, 
      'bracket-bottom' => $bottom_bracket,
      'bracket-champion' => array_merge($champion_match, $first_loss),
    );
  }
  
  /**
   * This function looks at the previous round and sets the winners path. On
   * the current round it sets the previous path for each contestant, (the 
   * match the contestant came from). This function is typically called when
   * a round is being built.
   * 
   * @param (array) $matches 
   *   The built up array structure of matches to modify.
   * @param (int) $current_round 
   *   The current round number that is being built.
   * @param (int) $current_match
   *   The current match number that is being build
   */
  protected function set_match_path($matches, $current_round, $current_match) {
    $previous_round = $current_round - 1;
    // Previous Match number.
    $pm = $current_match * 2;
    $previous_match = $pm - 1;
    
    if (array_key_exists('round-'. $previous_round, $matches)) {      
      if ($this->same_bracket) {
        $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'round-'. $current_round .'_match-'. $current_match . '_contestant-1';
        $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'round-'. $current_round .'_match-'. $current_match . '_contestant-2';
        $matches['round-'. $previous_round]['match-'. $previous_match]['loser'] = 'bracket-bottom_round-[bottom-round]_match-'. $current_match . '_contestant-2';
        $matches['round-'. $previous_round]['match-'. $pm]['loser'] = 'bracket-bottom_round-[bottom-round]'. $current_round .'_match-'. $current_match . '_contestant-2';
        $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'_match-'. $previous_match;
        $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'round-'. $previous_round .'_match-'. $pm;
      }
      // New contestants in round are not from the same bracket. The winner
      // from previous round will play the new contestants from other bracket.
      else {
        // Get the previous top round
        $top_round = array_search($current_round, $this->bracket_alignment) + 1;
        $matches['round-'. $previous_round]['match-'. $current_match]['winner'] = 'round-'. $current_round .'_match-'. $current_match . '_contestant-1';
        $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'round-'. $previous_round .'_match-'. $current_match;
        $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'bracket-top_round-'. $top_round .'_match-'. $current_match;
      }
    }
    
    // Setup the first round of the bottom bracket
    if ($current_round == 1 && !$this->same_bracket) {
      $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'bracket-top_round-'. $current_round .'_match-'. $previous_match;
      $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'bracket-top_round-'. $current_round .'_match-'. $pm;
    }
    
    return $matches;
  }
  
  private function build_bottom_rounds($num_contestants) {
    // First round of play in losers bracket
    $loser_contestants = $num_contestants /= 2;
    $num_bottom_round = 1;
    $matches = array();
    
    // This round aligns with the top bracket
    $this->bracket_alignment[] = $num_bottom_round;
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
        // This round aligns with the top bracket
        $this->bracket_alignment[] = $num_bottom_round;
        
        $this->build_round($matches, $loser_contestants, $num_bottom_round);
      }
      else {
        $num_bottom_round++;
        $this->losers_only = true;
        $this->same_bracket = true;
        $this->build_round($matches, $winning_losers, $num_bottom_round);
        // This round did not involve anyone from the winners bracket. Set the
        // number of contestants to equal only the teams that played.
        $loser_contestants = $winning_losers;
        $num_contestants = $winning_losers;
        $this->losers_only = false;
        $this->same_bracket = false;
      } 
    }
    $this->num_bottom_rounds = $num_bottom_round;
    return $matches;
  }
  
  /**
   * Replaces the loser paths in the top bracket with real locations
   */
  private function fix_top_loser_path(&$bracket) {
    foreach ($bracket as $round_id => &$round) {
      foreach ($round as &$match) {
        $id_parts = explode('-', $round_id);
        $one_back = $id_parts[1] - 1;
        $bottom_round = $this->bracket_alignment[$one_back];
        if (array_key_exists('loser', $match)) {
          $match['loser'] = str_replace('[bottom-round]', $bottom_round, $match['loser']);
        }
      }
    }
  }
}