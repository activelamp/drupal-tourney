<?php

/**
 * @file
 * Single elimination controller.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class SingleElimination implements TourneyFactoryInterface {

  protected $contestants;
  protected $rounds;
  protected $matches;

  /**
   * Constructor
   *
   * @todo The methods in this constuctor should be moved out and called
   *   explicitly when building tournaments.
   */
  public function __construct($contestants = 0) {
    if ($contestants > 0) {
      $this->calculateRounds($contestants);
    }
  }

  /**
   * Build the Single Elimination matchlist
   *
   * @return $matches
   *   The matches array completely built out.
   */
  public function build() {
    $matches = array();
    foreach ( range(1, $this->rounds) as $round ) {
      foreach ( range(1, $this->contestants / pow(2, $round)) as $match )
        $matches[] = array('bracket' => 'top', 'round' => $round, 'match' => $match);
    }
    $this->matches = $matches;
    return $this->matches;
  }

  /**
   * Rounds contestants up to the nearest power of two, and also sets and returns the number of rounds
   *
   * @return $rounds
   *   Number of rounds needed from the given contestants
   */
  public function calculateRounds($contestants = NULL) {
    // Populate contestants with our internal value if no argument given
    if ( $contestants == NULL ) $contestants = $this->contestants; 
    // @todo: perhaps change this to a maximum contestants with validation to keep it a power of two?
    $max_contestants = pow(2, MAXIMUM_ROUNDS);
    // Display an error if the maximum was pushed over
    if ( $contestants > $max_contestants ) {
      drupal_set_message(check_plain(t('Tournaments can only be !num rounds at the most with !player contestants. Some teams will not be able to play.', 
        array('!num' => MAXIMUM_ROUNDS, '!player' => $maximum_contestants))), 'warning');
      $contestants = $max_contestants;
    }
    // ceil(log2(n)) will get up the minimum number of rounds required for n contestants 
    $this->rounds = ceil(log($contestants, 2));
    // The rounded round count will reaffirm our contestants are a power of two
    $this->contestants = pow(2, $this->rounds);
    return $this->rounds;
  }

  /**
   * Sets the winner property and saves tournament.
   *
   * Retrieves rankings and sorts the list by total number of winnings. Sets
   * winner to the first contestant in the ranking list.
   *
   * @param TourneyTournament $tournament
   *
   * @return TourneyTournament $this
   *   Returns $this for chaining.
   */
  public function determineWinner($tournament) {
    $ranks = $tournament->fetchRanks();
    $standings = $tournament->getStandings();

    // todo : remove quick hack, implement custom uasort callback.
    foreach ($standings as $key => $standing) {
      $winners[$key] = $standing['wins'];
    }
    arsort($winners);

    $keys = array_keys($winners);
    $tournament->winner = $keys[0];
    //$tournament->tournamentWinner = $keys[0]; (is private);
    $tournament->save();

    return $this;
  }

  /**
   * Report if a tournament is finished.
   *
   * @param TourneyTournament $tournament
   *
   * @return bool $finished
   *   Will report TRUE if the tournament is finished, FALSE if not.
   */
  function isFinished($tournament) {
    $matches = tourney_match_load_multiple($tournament->getMatchIds());
    if (!empty($matches)) {
      foreach ($matches as $match) {
        // Delegate the checking to the match to see if each match is finished
        if (!$match->isFinished()) {
          return FALSE;
        }
      }
      return TRUE;
    }
    throw new Exception(t('There are no matches for this tournament'));
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
  public function getNextMatch($place, $direction = NULL) {
    // @todo find a better way to count matches
    $matches = $this->contestants - 1;
    // If it's the last match, it doesn't go anywhere
    if ( $place == $matches - 1 ) return NULL;
    // Otherwise some math!
    return ( ($matches + 1) / 2 ) + floor($place / 2);
  }
}