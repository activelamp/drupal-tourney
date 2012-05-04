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
  public function build() {
    $matches = parent::build();
    $matches = array_merge($matches, $this->buildBottomBracket());
    $matches[] = array('bracket' => 'champion', 'round' => 1, 'match' => 1);
    $matches[] = array('bracket' => 'champion', 'round' => 2, 'match' => 1);
    $this->matches = $matches;
    return $this->matches;
  }

  protected function buildBottomBracket() {
    $matches = array();
    $rounds = ( log($this->contestants, 2) - 1 ) * 2;
    foreach ( range(1, $rounds) as $round ) {
      $er = ceil($round/2);
      $m = $this->contestants / pow(2, $er+1);
      foreach ( range(1, $m) as $match ) {
        $matches[] = array('bracket' => 'bottom', 'round' => $round, 'match' => $match);
      }
    } 
    return $matches;
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
  *   @todo I should change this to /2 to begin with, but for now it's the full number of bottom matches
  * @return $series
  *   Array of numbers
  */
  private function magicSeries($until) {
    $series = array();
    $i = 0;
    // We're working to 8 if until is 16, 4 if until is 8 
    while ( $i < $until / 2 ) {
      // Add in this next double entry of numbers
      $series[] = ++$i;
      $series[] = $i;
      // If it's a power of two, throw in that many numbers extra
      if ( ($i & ($i - 1)) == 0 )
        foreach ( range(1, $i) as $n ) $series[] = $i;
    }
    // Remove the unnecessary last element in the series (which is the start of the next iteration) 
    array_pop($series);
    // Reverse it so we work down
    return array_reverse($series);
  }
}
