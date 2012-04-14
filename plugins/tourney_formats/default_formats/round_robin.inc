<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class RoundRobin implements TourneyMatchInterface {

  protected $num_contestants;
  protected $matches;
  protected $spots;

  /**
   * Constructor
   */
  public function __construct($num_contestants) {
    $this->num_contestants = $num_contestants;
  }

  /**
   * Build an array with all possible matches.
   * @see http://www.ehow.com/how_5796594_create-round_robin-schedule.html
   *
   * With round robin tournaments the teams switch off between being the "home"
   * team and the "away" team.  To accomodate this, assume the following is true:
   *   HOME TEAM = 'contestant-1'
   *   AWAY TEAM = 'contestant-2'
   *
   */
  public function build() {
    $matches = array();
    // Ensure we have an even number of spots
    $spots = $this->num_contestants % 2 ? $this->num_contestants + 1 : $this->num_contestants;
    $this->spots = $spots;

    // Number of rounds is (n - 1), n being contestants
    $rounds = $spots - 1;

    for ($r=1;$r<=$rounds;$r++) {
      for ($m=1;$m<=$spots/2;$m++) {
        $matches['bracket-roundrobin']['round-' . $r]['match-' . $m]['contestant-1'] = $r == 1 ? 'manual' : '';
        $matches['bracket-roundrobin']['round-' . $r]['match-' . $m]['contestant-2'] = $r == 1 ? 'manual' : '';

        $matches = $this->set_match_path($matches, $r, $m);
      }
    }
    return array(
      'bracket-roundrobin' => $matches,
    );
  }

  /**
   * Figure out where each team needs to go next based on standard round robin
   * logic. Build the array that determines the placeholder spots.
   *
   * @param $spots
   *   The (even) number of players in the tournament.
   */
  private function placeholders($spots) {
    static $placeholders = array();

    if (empty($placeholders)) {
      $num_rounds = $spots - 1;
      $rounds = array();

      for ($round = 0; $round < $num_rounds; $round++) {
        for ($index = 0; $index < $spots / 2; $index++) {

          // Create the ascending array value
          $local_key = ($index != 0) * ($index - $round) +
            (($index != 0) && (($index != 0) * ($index - $round) <= 0)) * $num_rounds;

          // Create the descending array value
          $away_key = $num_rounds - $index - $round +
            (($index != 0) && ($num_rounds - $index - $round <= 0)) * $num_rounds;

          // The home team and visitor team switch every other game
          $rounds[$round][] = $round % 2 ? array($away_key, $local_key) : array($local_key, $away_key);
        }
      }

      $placeholders = $rounds;
    }
    return $placeholders;
  }

  /**
   * There is a specific order that the matches are played. The fields that
   * track movement were initially built for winner path and loser path. In
   * round robin tournaments the path the teams take in the tournament do not
   * depend on who wins the match, it is already determined at the start of
   * the tournament. To accomodate for this, assume the following is true:
   *   CONTESTANT 1 NEXT GAME = winner
   *   CONTESTANT 2 NEXT GAME = loser
   *
   * @todo Fix the name of the db column paths so that they are named more generic.
   */
  protected function set_match_path($matches, $current_round, $current_match) {
    $rounds = $this->spots - 1;
    if ($current_round < $rounds) {
      $placeholders = $this->placeholders($this->spots);

      $matches['bracket-roundrobin']['round-' . $current_round]['match-' . $current_match]['winner'] = $this->find_next($current_round, $placeholders[$current_round - 1][$current_match - 1][0]);
      $matches['bracket-roundrobin']['round-' . $current_round]['match-' . $current_match]['loser'] = $this->find_next($current_round, $placeholders[$current_round - 1][$current_match - 1][1]);
    }
    return $matches;
  }

  /**
   * Return the path of the next match based on the placeholder value passed
   * in. This function looks for the place holder value in the next round.
   *
   * @param $round
   *   The round to find the placeholder value in.
   * @param $placeholder_value
   *   The number corresponding to the seed position in the tournament.
   * @return
   *   Returns a full path string to the next placement.
   */
  private function find_next($round, $placeholder_value) {
    $placeholders = $this->placeholders($this->spots);
    foreach ($placeholders[$round] as $key => $matches) {
      $pos = array_search($placeholder_value, $matches);
      if ($pos !== FALSE) {
        $r = $round + 1;
        $m = $key + 1;
        $c = $pos + 1;
        return 'bracket-roundrobin_round-' . $r . '_match-' . $m . '_contestant-' . $c;
      }
    }
  }

  /**
   *
   */
  public function determine_winner($tournament) {
    module_load_include('inc', 'tourney', 'theme/type/roundrobin');
    $ranks = $tournament->fetchRanks();
    $contestants = tourney_get_contestants($tournament);
    $standings = tourney_get_standings($tournament);
    uasort($standings, 'tourney_roundrobin_standings_sort');

    if (TOURNEY_DEBUG_EXTENDS) {
      dpm($contestants);
      dpm($standings);
      dpm($ranks);
    }

    $keys = array_keys($standings);
    $tournament->winner = $keys[0];
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

    // If all matches are finished then the tournament is finished.
    $matches = TourneyMatch::loadTourneyMatch($tournament->getMatchIds());
    if (!empty($matches)) {
      $finished = TRUE;
      foreach ($matches as $match) {
        $finished = ($finished) ? $match->isFinished() : FALSE;
      }
    }

    return $finished;
  }
}