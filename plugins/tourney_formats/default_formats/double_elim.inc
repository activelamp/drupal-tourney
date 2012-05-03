<?php

/**
 * @file
 * A Double Elimination class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class DoubleElimination extends SingleElimination implements TourneyFactoryInterface {

  protected $num_bottom_rounds;
  // Set a variable when contestants in round are from the same bracket.
  private $same_bracket = TRUE;
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
  protected function build_rounds() {
    $this->top_bracket = parent::build_rounds();
    // Set a flag for building bottom bracket.
    $this->same_bracket = FALSE;
    $this->bottom_bracket = $this->build_bottom_rounds();
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
    $matches['round-' . $current_round]['match-' . $current_match]['this_match'] = 'bracket-' . $bracket . '_round-' . $current_round . '_match-' . $current_match; //this was needed

    $previous_round = $current_round - 1;
    // Previous Match number.
    $pm = $current_match * 2;
    $previous_match = $pm - 1;

    // Champion bracket is always the same structure, hardcoding.
    if ($bracket == 'champion') {
      $top_last_round = count($this->top_bracket['bracket-top']);
      $bottom_last_round = count($this->bottom_bracket);

      // Set the last round of top bracket
      foreach ($this->top_bracket['bracket-top']['round-' . $top_last_round] as &$match_arr) {
        $match_arr['winner'] = 'bracket-champion_round-1_match-1_contestant-1';
        $match_arr['loser'] = 'bracket-bottom_round-' . $bottom_last_round . '_match-1_contestant-1';
      }

      // Set the last round of bottom bracket
      foreach ($this->bottom_bracket['round-' . $bottom_last_round] as &$match_arr) {
        $match_arr['winner'] = 'bracket-champion_round-1_match-1_contestant-2';
      }
    }

    // If we're in the top bracket, set a variable to be used for loser destination
    $loser_bracket = ($bracket == 'top') ? 'bottom' : NULL;
    // If we're in the bottom bracket, set a variable to be used for previous destination.
    $winner_bracket = ($bracket == 'bottom') ? 'top' : NULL;
    if (array_key_exists('round-' . $previous_round, $matches)) {
      if ($this->same_bracket) {
        $matches['round-' . $previous_round]['match-' . $previous_match]['winner'] = 'bracket-' . $bracket . '_round-' . $current_round . '_match-' . $current_match . '_contestant-1';
        $matches['round-' . $previous_round]['match-' . $pm]['winner'] = 'bracket-' . $bracket . '_round-' . $current_round . '_match-' . $current_match . '_contestant-2';
        $matches['round-' . $previous_round]['match-' . $previous_match]['loser'] = (isset($loser_bracket))
          ? 'bracket-' . $loser_bracket . '_round-[bottom-round]_match-' . $current_match . '_contestant-1'
          : NULL;
        $matches['round-' . $previous_round]['match-' . $pm]['loser'] = (isset($loser_bracket))
          ? 'bracket-' . $loser_bracket . '_round-[bottom-round]_match-' . $current_match . '_contestant-[bottom-contestant]'
          : NULL;
        $matches['round-' . $current_round]['match-' . $current_match]['previous-1'] = 'bracket-' . $bracket . '_round-' . $previous_round . '_match-' . $previous_match;
        $matches['round-' . $current_round]['match-' . $current_match]['previous-2'] = 'bracket-' . $bracket . '_round-' . $previous_round . '_match-' . $pm;
      }
      // New contestants in round are not from the same bracket. The winner
      // from previous round will play the new contestants from other bracket.
      else {
        // Get the previous top round
        $top_round = array_search($current_round, $this->bracket_alignment) + 1;
        $matches['round-' . $previous_round]['match-' . $current_match]['winner'] = 'bracket-' . $bracket . '_round-' . $current_round . '_match-' . $current_match . '_contestant-2';
        $matches['round-' . $current_round]['match-' . $current_match]['previous-1'] = 'bracket-' . $bracket . '_round-' . $previous_round . '_match-' . $current_match;
        $matches['round-' . $current_round]['match-' . $current_match]['previous-2'] = 'bracket-' . $winner_bracket . '_round-' . $top_round . '_match-' . $current_match;
      }
    }

    // Setup the first round of the bottom bracket
    if ($current_round == 1 && !$this->same_bracket) {
      $matches['round-' . $current_round]['match-' . $current_match]['previous-1'] = 'bracket-' . $winner_bracket . '_round-' . $current_round . '_match-' . $previous_match;
      $matches['round-' . $current_round]['match-' . $current_match]['previous-2'] = 'bracket-' . $winner_bracket . '_round-' . $current_round . '_match-' . $pm;
    }

    return $matches;
  }

  private function build_bottom_rounds() {
    // First round of play in losers bracket
    $loser_contestants = $this->contestants /= 2;
    $num_contestants = $this->contestants;
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
        $this->losers_only = TRUE;
        $this->same_bracket = TRUE;
        $this->build_round($matches, $winning_losers, $num_bottom_round, 'bottom');
        // This round did not involve anyone from the winners bracket. Set the
        // number of contestants to equal only the teams that played.
        $loser_contestants = $winning_losers;
        $num_contestants = $winning_losers;
        $this->losers_only = FALSE;
        $this->same_bracket = FALSE;
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
              $loser_parts[2] = 'match-' . $new_match;
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
   * Overrides SingleElemination::isFinished().
   *
   * If the top two ranking contestants are not tied in their number of wins
   * then we do not require the final match to be finished.
   *
   * @see SingleElimination::isFinished().
   */
  public function isFinished($tournament) {
    $finished = parent::isFinished($tournament);

    // Parent is authoritative if it reports tournament as already finished.
    if (!$finished) {
      $ranks = $tournament->fetchRanks();
      // If we have one outstanding match, tournament may be finished if no 
      // tie-breaker is required.
      if ((array_key_exists('NA', $ranks['match_wins'])) && ($ranks['match_wins']['NA'] == 1)) {
        $keys = array_keys($ranks['match_wins']);
        // If first and second place contestants do not report same win count.
        if ($ranks['match_wins'][$keys[0]] != $ranks['match_wins'][$keys[1]]) {
          $finished = TRUE;
        }
      }
    }

    return $finished;
  }

 /**
  * Given a match place integer, returns the next match place based on either 'winner' or 'loser' direction
  *
  * @param $place
  *   Match placement, zero-based. round 1 match 1's match placement is 0
  * @param $direction
  *   Either 'winner' or 'loser'
  * @return $place
  *   Match placement of the desired match, otherwise NULL 
  */
  public function getNextMatch($place, $direction = 'winner') {
    // @todo find a better way to count matches
    $contestants = $this->contestants;
    // Set up our handy values
    $matches = $contestants * 2 - 1;
    $top_matches = $contestants - 1;
    $bottom_matches = $top_matches - 1;

    if ( $direction == 'winner' ) {
      // Top Bracket
      if ( $place < $top_matches ) {
        // Last match in the top bracket goes to the champion bracket
        if ( $place == $bottom_matches ) return $matches - 2;
        return parent::getNextMatch($place);
      }
      // Champion Bracket(s)
      elseif ( $place >= $matches - 2 ) {
        // Last match goes nowhere
        if ( $place == $matches - 1 ) return NULL;
        return $place + 1;
      }
      // Bottom Bracket
      else {
        // Get out series to find out how to adjust our place
        $series = $this->magicSeries($bottom_matches);
        return $place + $series[$place-$top_matches];
      }
    }
    elseif ( $direction == 'loser' ) {
      // Top Bracket
      if ( $place < $top_matches ) {
        // If we're in the first round of matches, it's rather simple
        if ( $place < $contestants / 2 ) 
          return parent::getNextMatch($place) + ($bottom_matches/2);          
        // Otherwise, more magical math to determine placement
        return $place + $top_matches - pow(2, floor(log($top_matches - $place, 2)));
      }
    }
    return NULL;
  }

 /**
  * This is a special function that I could have just stored as a fixed array, but I wanted it to scale
  * It creates a special series of numbers that affect where loser bracket matches go
  *
  * @param $until
  *   Number of items in the sequence to generate to  
  * @return $series
  *   Array of numbers
  */
  private function magicSeries($until) {
    $series = array(1, 1);
    $i = 1;
    while ( count($series) < $until ) {
      //pad from n to n*2 with double integers
      $series[] = $i;
      $next = $i * 2;
      while ( ++$i < $next ) {
        $series = array_merge($series, array($i, $i));
      }
      $series[] = $i;
      //fill in n * n
      foreach ( range(1, $i) as $n ) $series[] = $i;
    }
    // Reverse it so we work down from the highest number
    return array_reverse($series);
  }
}