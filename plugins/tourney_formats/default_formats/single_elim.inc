<?php

/**
 * @file
 * Single elimination controller.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class SingleElimination implements TourneyFactoryInterface {

  protected $num_contestants;
  protected $num_rounds;
  protected $matches;

  /**
   * Constructor
   *
   * @todo The methods in this constuctor should be moved out and called
   *   explicitly when building tournaments.
   */
  public function __construct($num_contestants = 0) {
    if ($num_contestants > 0) {
      $this->calculate_rounds($num_contestants);
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
    $this->matches = $this->build_rounds($this->num_contestants);
    return $this->matches;
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
  protected function build_rounds($num_contestants) {
    $matches = array();
    $this->calculate_rounds($num_contestants);
    $num_rounds = $this->num_rounds;

    for ($r = 1; $r <= $num_rounds; ++$r) {
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
    return $this->num_contestants;
  }

  protected function set_message() {
    drupal_set_message(check_plain(get_class($this) . ' with ' . $this->num_contestants . ' contestants and ' . $this->num_rounds . ' rounds.'));
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

  private function calculate_rounds($num_contestants) {
    // Make sure we don't have too many contestants.
    $maximum_contestants = pow(2, MAXIMUM_ROUNDS);
    if ($num_contestants > $maximum_contestants) {
      drupal_set_message(check_plain(t('Tournaments can only be !num rounds at the most with !player contestants. Some teams will not be able to play.', array('!num' => MAXIMUM_ROUNDS, '!player' => $maximum_contestants))), 'warning');
      $num_contestants = $maximum_contestants;
    }

    // Store values of exponents with base 2
    $possible_count = array();

    // Figure out how many rounds we're going to have.
    for ($i = 1; $i <= MAXIMUM_ROUNDS; ++$i) {
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
    for ($i = 0; $i <= count($possible_count); ++$i) {
      // Find between what valid possible counts the number of contestants is in.
      if ($num_contestants > $possible_count[$i] && $num_contestants < $possible_count[$i + 1]) {
        // Set the number of contestants to the higher number
        $new_num_contestants = $possible_count[$i + 1];
        // Calculate number of rounds now that we fixed contestant count.
        $this->calculate_rounds($new_num_contestants);
        return;
      }
    }
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
    $standings = tourney_get_standings($tournament);

    // todo : remove quick hack, implement custom uasort callback.
    foreach ($standings as $key => $standing) {
      $winners[$key] = $standing['wins'];
    }
    arsort($winners);

    $keys = array_keys($winners);
    $tournament->winner = $keys[0];
    //$tournament->tournamentWinner = $keys[0]; (is private);
    $tournament->save();
    drupal_set_message(t('Tournament !format is complete!.', array('!format' => $tournament->format)), 'status');

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
}