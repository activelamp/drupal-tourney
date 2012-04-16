<?php

/**
 * @file
 * A Single Elimination class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class SingleElimination implements TourneyMatchInterface {

  protected $num_contestants;
  protected $num_rounds;
  protected $matches;

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
    // @todo document this function

    $matches = array(array(1, 2));
    $num_matches = pow(2, SingleElimination::calculate_rounds($num_contestants)) / 2;

    $i = 0;
    while(count($matches) < $num_matches) {
      $i++;

      $sum = (count($matches) * 4) + 1;

      $new_matches = array();
      foreach ($matches as $match) {
        foreach ($match as $contestant) {
          if ($sum - $contestant <= $num_contestants) {
            $new_matches[] = array($contestant, $sum - $contestant);
          }
          else {
            $new_matches[] = array($contestant, NULL);
          }
        }
      }
      $matches = $new_matches;
    }
    return $matches;
  }

  /**
   * Constructor
   */
  public function __construct($num_contestants = NULL) {
    if ($num_contestants) {
      $this->set_num_contestants($num_contestants);
      drupal_set_message(get_class($this) .' with '. $this->num_contestants .' contestants and '. $this->num_rounds .' rounds.');
    }
    else {
      drupal_set_message(get_class($this) .' with no contestants. Remember to set number of contestants.');
    }
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
   * Build the nested matches array with all possible matches.
   *
   * @return $matches
   *   The matches array completely built out.
   */
  public function build() {
    $matches = array();
    $num_rounds = $this->num_rounds;
    $num_contestants = $this->num_contestants;

    foreach (range(1, $num_rounds) as $round) {
      $positions = $this::calculate_seed_positions($num_contestants);
      $matches['round-'. $round] = array();

      foreach ($positions as $match => $position) {
        if ($round == 1) {
          $matches['round-'. $round]['match-'. ($match + 1)] = array(
            'contestant-1' => (empty($position[0]) ? 'bye' : 'manual'),
            'contestant-2' => (empty($position[1]) ? 'bye' : 'manual'),
          );
        }
        else {
          $matches['round-'. $round]['match-'. ($match + 1)] = array(
            'contestant-1' => '',
            'contestant-2' => '',
          );
        }
        $matches = $this->set_match_path($matches, 'top', $round, ($match + 1));
      }

      // Bump the number of contestants down to the next level in the tournament
      $num_contestants = pow(2, $num_rounds - $round);
    }

    $this->matches = array(
      'bracket-top' => $matches,
    );

    $this->matches = $this->set_bye_manuals($this->matches, 'top', 2);

    return $this->matches;
  }

  /**
   * 
   */
  public function determine_winner($tournament) {
    $ranks = $tournament->fetchRanks();
    $contestants = tourney_get_contestants($tournament);
    $standings = tourney_get_standings($tournament);

    // todo : remove quick hack, implement custom uasort callback.
    foreach ($standings as $key => $standing) {
      $winners[$key] = $standing['wins'];
    }
    arsort($winners);

    if (TOURNEY_DEBUG_EXTENDS) {
      dpm($contestants);
      dpm($standings);
      dpm($ranks);
      dpm($winners);
    }

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
  function is_finished($tournament) {
    if (!property_exists($tournament, 'id') || !TourneyTournament::exists($tournament->id)) {
      throw new Exception(t('Required parameter is not correct.'));
    }

    $finished = FALSE;

    $matches = TourneyMatch::loadTourneyMatch($tournament->getMatchIds());
    if (!empty($matches)) {
      $finished = TRUE;
      foreach ($matches as $match) {
        $finished = ($finished) ? $match->isFinished() : FALSE;
      }
    }

    return $finished;
  }

  protected function set_match_path($matches, $bracket, $current_round, $current_match) {
    // Set the winner path in the previous round
    $previous_round = $current_round-1;
    $matches['round-'. $current_round]['match-'. $current_match]['this_match'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match;
    if (array_key_exists('round-'. $previous_round, $matches)) {
      // Previous Match number.
      $pm = $current_match * 2;
      $previous_match = $pm - 1;
      $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match . '_contestant-1';
      $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match . '_contestant-2';
      $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'bracket-'. $bracket .'_round-'. $previous_round .'_match-'. $previous_match;
      $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'bracket-'. $bracket .'_round-'. $previous_round .'_match-'. $pm;
    }
    return $matches;
  }

  /**
   * Determine if byes in the previous round create manual slots in this round
   */
  protected function set_bye_manuals($matches, $bracket = 'top', $round = 2) {
    foreach ($matches['bracket-'. $bracket]['round-'. $round] as $m => $match) {
      // Look at each match in this round

      foreach (array('previous-1', 'previous-2') as $previous) {
        // set_match_path() gave us a path to follow back to the matches that
        // fed into this one.

        // Navigate to the previous match. $child will be the previous match.
        $parents = explode('_', $match[$previous]);
        $child = $matches;
        while ($parent = array_shift($parents)) {
          $child = $child[$parent];
        }

        if ($child['contestant-2'] == 'bye') {
          // Only contestant 2 can be a bye. If this match was a bye, set the
          // current contestant to manual select

          $current_contestant = ($previous == 'previous-1' ? 'contestant-1' : 'contestant-2');
          $matches['bracket-'. $bracket]['round-'. $round][$m][$current_contestant] = 'manual';
        }
      }
    }
    return $matches;
  }
}