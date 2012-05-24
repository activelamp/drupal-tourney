<?php

/**
 * @file
 * A Single Elimination class for tournaments.
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
      $this->num_contestants = $num_contestants;
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
    $this->set_num_contestants($num_contestants);
    $num_rounds = $this->num_rounds;

    for ($r=1;$r<=$num_rounds;$r++) {
      $matches['round-' . $r] = array();
      $this->build_round($matches, $num_contestants, $r);

      // Bump the number of contestants down to the next level in the tournament
      $num_contestants = pow(2, $num_rounds - $r);
    }

    $matches = $this->set_bye_manuals($matches);

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
    $positions = $this::calculate_seed_positions($contestants);
    foreach ($positions as $match => $position) {
      $match += 1;
      if ($round == 1) {
        $matches['round-'. $round]['match-'. $match] = array(
          'contestant-1' => (empty($position[0]) ? 'bye' : 'manual'),
          'contestant-2' => (empty($position[1]) ? 'bye' : 'manual'),
        );
      }
      else {
        $matches['round-'. $round]['match-'. $match] = array(
          'contestant-1' => '',
          'contestant-2' => '',
        );
      }
      $matches = $this->set_match_path($matches, $bracket, $round, $match);
    }
  }

  /**
   * Calculate number of rounds given a number of contestants
   */
  public static function calculate_rounds($num_contestants) {
    return ceil(log($num_contestants, 2));
  }

  /**
   * Calculate the starting seed positions.
   *
   * @return $matches
   *   An array of match pairs with seed numbers, or NULL for bye slots.
   */
  public static function calculate_seed_positions($num_contestants) {    
    // Initialize a the first match in the first round matches.
    $first_round_matches = array(array(1, 2));
    $num_first_round_matches = pow(2, SingleElimination::calculate_rounds($num_contestants)) / 2;
    
    // Continue to find seed positions until we have all the first round matches
    // populated, based on the number of "real" matches (w/o byes).
    // 
    // $first_round_matches array contains that matches that have already been
    // populated from this method.
    while(count($first_round_matches) < $num_first_round_matches) {
      // The $multiplier is the number we need to multiply the number of currently
      // built rounds to get the seed position number of the very last place.
      $multiplier = 4;
      // Last seed position plus 1
      $last_seed_position = (count($first_round_matches) * $multiplier) + 1;

      $new_matches = array();
      // Go through each match already created and update according to what the
      // latest seed positions being added to tournament.
      foreach ($first_round_matches as $match) {
        foreach ($match as $seed_position) {
          // Match the current seed_position being processed with the
          // last_seed_position - this seed_position. If the last seed position
          // doesn't exist, create a bye.
          if ($last_seed_position - $seed_position <= $num_contestants) {
            $new_matches[] = array($seed_position, $last_seed_position - $seed_position);
          }
          else {
            $new_matches[] = array($seed_position, NULL);
          }
        }
      }
      $first_round_matches = $new_matches;
    }
    return $first_round_matches;
  }

  /**
   * Set the number of contestants
   */
  public function set_num_contestants($num_contestants) {
    // Make sure we don't have too many contestants.
    $maximum_contestants = pow(2, MAXIMUM_ROUNDS);
    if ($num_contestants > $maximum_contestants) {
      drupal_set_message(t('Tournaments can only be !num rounds at the most with !player contestants. Some teams will not be able to play.', array('!num' => MAXIMUM_ROUNDS, '!player' => $maximum_contestants)), 'warning');
      $num_contestants = $maximum_contestants;
    }

    $this->num_contestants = $num_contestants;
    $this->num_rounds = $this::calculate_rounds($num_contestants);
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
    drupal_set_message(t('Tournament ' . $tournament->format. ' is complete!.'), 'status');

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
   * Determine if byes in the previous round create manual slots in this round
   */
  protected function set_bye_manuals($matches, $round = 2) {
    foreach ($matches['round-'. $round] as $m => $match) {
      // Look at each match in this round

      foreach (array('previous-1', 'previous-2') as $previous) {
        // set_match_path() gave us a path to follow back to the matches that
        // fed into this one.

        // Navigate to the previous match. $child will be the previous match.
        $parents = explode('_', $match[$previous]);
        array_shift($parents);  // shift off bracket
        $child = $matches;
        while ($parent = array_shift($parents)) {
          $child = $child[$parent];
        }

        if ($child['contestant-2'] == 'bye') {
          // Only contestant 2 can be a bye. If this match was a bye, set the
          // current contestant to manual select

          $current_contestant = ($previous == 'previous-1' ? 'contestant-1' : 'contestant-2');
          $matches['round-'. $round][$m][$current_contestant] = 'manual';
        }
      }
    }
    return $matches;
  }
}