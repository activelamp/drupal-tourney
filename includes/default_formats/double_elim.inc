<?php

/**
 * @file
 * A Double Elimination class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class DoubleElimination extends SingleElimination implements TourneyMatch {

  public function build() {
    $top_bracket = parent::build();

    $num_contestants = parent::get_num_contestants();
    $bottom_bracket = parent::build_rounds($num_contestants / 2);
    return array('top-bracket' => $top_bracket, 'bottom-bracket' => $bottom_bracket);
  }
  
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
  
}