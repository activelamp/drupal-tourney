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
  
  private $top_bracket;
  private $bottom_bracket;
  
  
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
    $this->top_bracket = parent::build_rounds($num_contestants);
    // Set a flag for building bottom bracket.
    $this->same_bracket = false;
    $this->bottom_bracket = $this->build_bottom_rounds($num_contestants);
    // Fix the loser path in top bracket, now that the bottom bracket has been
    // built.
    $this->fix_top_loser_path($this->top_bracket);
    
    // The Champion bracket is the last round when both brackets converge. It
    // always looks the same.  Defining the bracket explicityly.
    $champion_match = $first_loss = array();
    $this->build_round($champion_match, 2, 1, 'champion');
    $this->build_round($first_loss, 2, 2, 'champion');
    
    return array(
      'bracket-top' => $this->top_bracket['bracket-top'], 
      'bracket-bottom' => $this->bottom_bracket,
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
   * @param (string) $bracket
   *   The bracket name for the match.
   * @param (int) $current_round 
   *   The current round number that is being built.
   * @param (int) $current_match
   *   The current match number that is being build
   */
  protected function set_match_path($matches, $bracket, $current_round, $current_match) {
    $matches['round-'. $current_round]['match-'. $current_match]['this_match'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match; //this was needed

    $previous_round = $current_round - 1;
    // Previous Match number.
    $pm = $current_match * 2;
    $previous_match = $pm - 1;
    
    // Champion bracket is always the same structure, hardcoding.
    if ($bracket == 'champion') {
      $top_last_round = count($this->top_bracket['bracket-top']);
      $bottom_last_round = count($this->bottom_bracket);
      
      // Set the last round of top bracket
      foreach ($this->top_bracket['bracket-top']['round-'. $top_last_round] as &$match_arr) {
        $match_arr['winner'] = 'bracket-champion_round-1_match-1_contestant-1';
        $match_arr['loser'] = 'bracket-bottom_round-'. $bottom_last_round .'_match-1_contestant-1';
      }
      
      // Set the last round of bottom bracket
      foreach ($this->bottom_bracket['round-'. $bottom_last_round] as &$match_arr) {
        $match_arr['winner'] = 'bracket-champion_round-1_match-1_contestant-2';
      }
    }
    
    // If we're in the top bracket, set a variable to be used for loser destination
    $loser_bracket = ($bracket == 'top') ? 'bottom' : NULL;
    // If we're in the bottom bracket, set a variable to be used for previous destination.
    $winner_bracket = ($bracket == 'bottom') ? 'top' : NULL;
    if (array_key_exists('round-'. $previous_round, $matches)) {      
      if ($this->same_bracket) {
        $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match . '_contestant-1';
        $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match . '_contestant-2';
        $matches['round-'. $previous_round]['match-'. $previous_match]['loser'] = (isset($loser_bracket)) 
          ? 'bracket-'. $loser_bracket .'_round-[bottom-round]_match-'. $current_match . '_contestant-1'
          : NULL;
        $matches['round-'. $previous_round]['match-'. $pm]['loser'] = (isset($loser_bracket)) 
          ? 'bracket-'. $loser_bracket .'_round-[bottom-round]_match-'. $current_match . '_contestant-[bottom-contestant]'
          : NULL;
        $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'bracket-'. $bracket .'_round-'. $previous_round .'_match-'. $previous_match;
        $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'bracket-'. $bracket .'_round-'. $previous_round .'_match-'. $pm;
      }
      // New contestants in round are not from the same bracket. The winner
      // from previous round will play the new contestants from other bracket.
      else {
        // Get the previous top round
        $top_round = array_search($current_round, $this->bracket_alignment) + 1;
        $matches['round-'. $previous_round]['match-'. $current_match]['winner'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match . '_contestant-2';
        $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'bracket-'. $bracket .'_round-'. $previous_round .'_match-'. $current_match;
        $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'bracket-'. $winner_bracket .'_round-'. $top_round .'_match-'. $current_match;
      }
    }
    
    // Setup the first round of the bottom bracket
    if ($current_round == 1 && !$this->same_bracket) {
      $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'bracket-'. $winner_bracket .'_round-'. $current_round .'_match-'. $previous_match;
      $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'bracket-'. $winner_bracket .'_round-'. $current_round .'_match-'. $pm;
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
    $this->build_round($matches, $loser_contestants, $num_bottom_round, 'bottom');
    
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
        
        $this->build_round($matches, $loser_contestants, $num_bottom_round, 'bottom');
      }
      else {
        $num_bottom_round++;
        $this->losers_only = true;
        $this->same_bracket = true;
        $this->build_round($matches, $winning_losers, $num_bottom_round, 'bottom');
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
   * Replaces the loser paths in the top bracket with real locations in the 
   * bottom bracket.  The first loser from the top bracket should always to go 
   * position one in bottom bracket unless in round one.
   * 
   * This function is called after the bottom bracket was built.  The bottom
   * bracket stores the alignment between the top bracket and bottom bracket
   * in a property of the object called $this->bracket_alignment.
   */
  private function fix_top_loser_path(&$bracket) {
    foreach ($bracket['bracket-top'] as $round_id => &$round) {
      // Need to increment match number every other match.
      $match_iterator = 1;
      
      foreach ($round as &$match) {
        $id_parts = explode('-', $round_id);
        $one_back = $id_parts[1] - 1;
        $bottom_round = $this->bracket_alignment[$one_back];
        if (array_key_exists('loser', $match)) {
          $match['loser'] = str_replace('[bottom-round]', $bottom_round, $match['loser']);
          // First round losers can go to slot 2, after that they always go to
          // slot 1.
          if ($bottom_round == 1) {
            $contestant = 2;
          }
          else {
            $contestant = 1;
            
            // Increment the match number by one. This is a seeded match from
            // the top bracket.
            $loser_parts = explode('_', $match['loser']);
            
            // Every other match needs to increment so we don't have two teams
            // in the same slot.
            if ($match_iterator != 1) {
              $match_num = substr($loser_parts[2], 6);
              $match_incrementer = $match_iterator - $match_num;
              // Put the loser string back together.
              $new_match = $match_num + $match_incrementer;
              $loser_parts[2] = 'match-'. $new_match;
              $match['loser'] = join("_", $loser_parts);
            }
            
            $match_iterator++;
          }
          $match['loser'] = str_replace('[bottom-contestant]', $contestant, $match['loser']);
        }
      }
    }
  }

  /**
   * Overrides SingleElemination::is_finished()
   *
   * @todo Determine if we need the tie-breaker match.
   */
  public function is_finished($tournament) {
    $finished = parent::is_finished($tournament);
    $finished_count = 0;

    // Parent is authoritative if it reports tournament as already finished.
    if (!$finished) {
      // Sum all completed matches.
      $matches_total = $tournament->matches_created;
      $matches = TourneyMatch::loadTourneyMatch($tournament->getMatchIds());
      $finish_count = 0;
      if (!empty($matches)) {
        foreach($matches as $match) {
          $finish_count += $match->isFinished();
        }
      }
      // If completed matches are one less than total matches then tournament
      // may be finished if no tie-breaker is required.
      if (($matches_total - $finished_count) == 1) {
        // TODO : Determine if we need the tie-breaker match.
      }
    }

    return $finished;
  }
}