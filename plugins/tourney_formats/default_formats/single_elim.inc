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
      $this->calculate_rounds($contestants);
    }
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
    $this->matches = $this->build_rounds($this->contestants);
    return $this->matches;
  }

  public function buildMatches() {
    $this->contestants = pow(2, ceil(log($this->contestants, 2)));

  }

  /**
   * Figure out how many rounds there should be based on $num_contestants,
   * then loop through and build all rounds.
   *
   * @param $num_contestants
   *   The number of contestants in the first round.
   * @return $matches
   *   The matches array completely built out.
   */
  protected function build_rounds() {
    $matches = array();

    $this->calculate_rounds($this->contestants);
    $num_contestants = $this->contestants;

    for ($r = 1; $r <= $this->rounds; ++$r) {
      $matches['round-' . $r] = array();
      $this->build_round($matches, $num_contestants, $r);
      $num_contestants /= 2;
    }

    return array(
      'bracket-top' => $matches,
    );
  }

  protected function set_match_path($matches, $bracket, $current_round, $current_match) {
    // Set the winner path in the previous round
    $previous_round = $current_round-1;
    $matches['round-' . $current_round]['match-' . $current_match]['this_match'] = 'bracket-' . $bracket . '_round-' . $current_round . '_match-' . $current_match;
    if (array_key_exists('round-' . $previous_round, $matches)) {
      // Previous Match number.
      $pm = $current_match * 2;
      $previous_match = $pm - 1;
      $matches['round-' . $previous_round]['match-' . $previous_match]['winner'] = 'bracket-' . $bracket . '_round-' . $current_round . '_match-' . $current_match . '_contestant-1';
      $matches['round-' . $previous_round]['match-' . $pm]['winner'] = 'bracket-' . $bracket . '_round-' . $current_round . '_match-' . $current_match . '_contestant-2';
      $matches['round-' . $current_round]['match-' . $current_match]['previous-1'] = 'bracket-' . $bracket . '_round-' . $previous_round . '_match-' . $previous_match;
      $matches['round-' . $current_round]['match-' . $current_match]['previous-2'] = 'bracket-' . $bracket . '_round-' . $previous_round . '_match-' . $pm;
    }
    return $matches;
  }

  /**
   * Getter for the number of contestants
   */
  protected function get_num_contestants() {
    return $this->contestants;
  }

  protected function build_round(&$matches, $contestants, $round, $bracket = 'top') {
    for ($match=1;$match<=$contestants/2;$match++) {
      $matches['round-' . $round]['match-' . $match] = array(
        'contestant-1' => ($round == 1) ? 'manual' : '',
        'contestant-2' => ($round == 1) ? 'manual' : '',
      );
      $matches = $this->set_match_path($matches, $bracket, $round, $match);
    }
  }

  public function calculate_rounds($contestants) { 
    $max_contestants = pow(2, MAXIMUM_ROUNDS);
    if ( $contestants > $max_contestants ) {
      drupal_set_message(check_plain(t('Tournaments can only be !num rounds at the most with !player contestants. Some teams will not be able to play.', array('!num' => MAXIMUM_ROUNDS, '!player' => $maximum_contestants))), 'warning');
      $contestants = $max_contestants;
    }
    $this->rounds = ceil(log($contestants, 2));
    $this->contestants = pow(2, $this->rounds);
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

  public function getNextMatch($place, $direction = NULL) {
    // @todo find a better way to count matches
    $matches = $this->contestants - 1;
    // If it's the last match, it doesn't go anywhere
    if ( $place == $matches - 1 ) return NULL;
    // Otherwise some math!
    return ( ($matches + 1) / 2 ) + floor($place / 2);
  }
}