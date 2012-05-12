<?php

/**
 * @file
 * Single elimination controller.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class SingleEliminationController extends TourneyController implements TourneyControllerInterface {

  protected $tournament;
  protected $slots;
  protected $rounds;
  protected $matches;

  /**
   * Constructor
   *
   * @todo The methods in this constuctor should be moved out and called
   *   explicitly when building tournaments.
   */
  public function __construct(TourneyTournament $tournament) {
    $this->tournament = $tournament;
    
    if (!empty($tournament->players) && $tournament->players > 0) {
      // Populate the object with some meta data.
      $this->calculateRounds($tournament->players);
    }
  }

  /**
   * Slots getter function
   *
   * @return $slots
   */
  public function getSlots() {
    return $this->slots;    
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
      foreach ( range(1, $this->slots / pow(2, $round)) as $match )
        $matches[] = array('bracket' => 'top', 'round' => $round, 'match' => $match);
    }
    $this->matches = $matches;
    return $this->matches;
  }

  /**
   * Rounds contestants up to the nearest power of two, and also sets and returns
   * the number of rounds
   *
   * @return $rounds
   *   Number of rounds needed from the given contestants
   */
  public function calculateRounds($contestants = NULL) {
    // Populate contestants with our internal value if no argument given
    if ( $contestants == NULL ) $contestants = $this->slots; 
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
    $this->slots = pow(2, $this->rounds);
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
    if ( $direction == 'loser' ) return NULL;
    // @todo find a better way to count matches
    $matches = $this->slots - 1;
    // If it's the last match, it doesn't go anywhere
    if ( $place == $matches - 1 ) return NULL;
    // Otherwise some math!
    return ( ($matches + 1) / 2 ) + floor($place / 2);
  }
  
  /**
   * Build an array with tournament structure data.
   *
   * @return $structure
   *   An array of the tournament structure and meta data.
   */
  public function structure($type = 'rounds') {
    switch ($type) {
      case 'rounds':
        return $this->structure = $this->buildBracketByRounds($this->slots);

      case 'tree':
        return $this->structure = $this->buildBracketByTree($this->slots);
    }
  }
  
  /**
   * Build bracket structure and logic.
   *
   * @param $slots
   *   The number of slots in the first round.
   * @return
   *   An array of bracket structure and logic.
   */
  protected function buildBracketByTree($slots) {
    $num_matches = ($slots * 2) - 1;
    $num_rounds = log($slots, 2);

    if (!is_int($num_rounds)) {
      // @todo: Acocunt for byes.
    }

    $tree = $this->buildMatch($slots, $round_num, $match_num);
    $tree['children'] = $this->buildChildren();

    return array(
      'bracket-main' => array(
        'title'    => t('Main Bracket'),
        'children' => $this->buildChildren($slots, $num_rounds, $num_matches),
      ),
    );
  }

  protected function buildChildren($slots, $round_num, $match_num) {
    $tree = $this->buildMatch($slots, $round_num, $match_num);

    if ($round_num > 1) {
      $child_match_num = ($match_num - ($slots / 2)) * 2;
      $tree['children'][] = $this->buildChildren($slots, $round_num - 1, $child_match_num - 1);
      $tree['children'][] = $this->buildChildren($slots, $round_num - 1, $child_match_num);
    }

    return $tree;
  }

  /**
   * Build bracket structure and logic.
   *
   * @param $slots
   *   The number of slots in the first round.
   * @return
   *   An array of bracket structure and logic.
   */
  protected function buildBracketByRounds($slots) {
    return array(
      'bracket-main' => array(
        'title'  => t('Main Bracket'),
      ) + $this->buildRounds($slots),
    );
  }

  /**
   * Build rounds.
   *
   * @param $slots
   *   The number of slots in the first round.
   * @return
   *   The rounds array completely built out.
   */
  protected function buildRounds($slots) {
    $rounds    = array();
    $round_num = 1;

    for ($slots_left = $slots; $slots_left >= 2; $slots_left /= 2) {
      $rounds['rounds']['round-' . $round_num] = $this->buildRound($slots_left, $round_num);
      $round_num++;
    }

    return $rounds;
  }

  protected function buildRound($slots, $round_num) {
    $round = array(
      'title' => t('Round ') . $round_num,
    );

    for ($match_num = 1; $match_num <= ($slots / 2); ++$match_num) {
      $round['matches']['match-' . $match_num] = $this->buildMatch($slots, $round_num, $match_num);
    }

    return $round;
  }

  protected function buildMatch($slots, $round_num, $match_num) {
    return array(
      'match' => $this->getMatch($round_num, $match_num),
      'previous_match_callback' => 'getPreviousMatch',
      'next_match_callback' => 'getNextMatch',
    );
  }
}