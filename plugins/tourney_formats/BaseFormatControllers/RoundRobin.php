<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */

class RoundRobinController extends TourneyController implements TourneyControllerInterface {

  protected $slots;
  protected $contestants;
  protected $matches;

  /**
   * Constructor
   */
  public function __construct(TourneyTournament $tournament) {
    $this->contestants = $tournament->players;
    // Ensure we have an even number of slots
    $slots = $this->contestants % 2 ? $this->contestants + 1 : $this->contestants;
    $this->slots = $slots;
  }

  public function getSlots() {
    return $this->slots;    
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
    // Number of rounds is (n - 1), n being contestants
    $rounds = $this->slots - 1;

    for ($r=1;$r<=$rounds;$r++) {
      for ($m=1;$m<=$this->slots/2;$m++) {
        $matches[] = array('bracket' => 'roundrobin', 'round' => $r, 'match' => $m);
      }
    }
    return $matches;
  }

  /**
   * Figure out where each team needs to go next based on standard round robin
   * logic. Build the array that determines the placeholder slots.
   *
   * @param $slots
   *   The (even) number of players in the tournament.
   */
  public function placeholders($slots) {
    static $matches = array();

    if ( empty($matches) ) {
      $matches = array();
      foreach ( range(1, $slots - 1) as $round ) {
        $list = range(2, $slots);
        $list = array_merge(array(1), array_slice(array_merge($list, $list), $slots-$round, $slots-1));
        foreach ( range(1, $slots / 2) as $match ) {
          $match = array($list[$match-1], $list[$slots-$match]);
          $matches[] = ( $round % 2 ) ? $match : array_reverse($match);
        }
      }
    }
    return $matches;
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
   *
   * @see tourney_roundrobin_standings_sort()
   */
  public function determineWinner($tournament) {
    module_load_include('inc', 'tourney', 'theme/type/roundrobin');
    $ranks = $tournament->fetchRanks();
    $standings = $tournament->getStandings();
    uasort($standings, 'tourney_roundrobin_standings_sort');

    $keys = array_keys($standings);
    $tournament->winner = $keys[0];
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
    if (!property_exists($tournament, 'id')) {
      throw new Exception(t('Required parameter is not correct.'));
    }

    $finished = FALSE;

    // If all matches are finished then the tournament is finished.
    $matches = tourney_match_load_multiple($tournament->getMatchIds());
    if (!empty($matches)) {
      $finished = TRUE;
      foreach ($matches as $match) {
        $finished = ($finished) ? $match->isFinished() : FALSE;
      }
    }

    return $finished;
  }

 /**
  * Given a match place integer and a direction, returns both the target match 
  * and slot
  *
  * @param $place
  *   Match placement, zero-based. round 1 match 1's match placement is 0
  * @param $slot
  *   Either 'home' or 'away'
  * @return $result
  *   Keyed array giving both the target match and slot
  */
  function getNextMatchSlot($place, $slot) {
    if ( !$slot ) return NULL;
    $placeholders = $this->placeholders($this->slots);
    // Get the current contestant slot number
    $id = $placeholders[$place][$slot-1];
    foreach ( $placeholders as $m => $slots ) {
      if ( $m <= $place ) continue;
      // Check for the next instance of it after the current match
      if ( in_array($id, $slots) ) {
        $slots = array_flip($slots);
        return array('match' => $m, 'slot' => $slots[$id]);
      }
    }
    return NULL;
  }

 /**
  * Given a match place integer, returns the next match place based on either 
  * 'home' or 'away' direction
  *
  * @param $place
  *   Match placement, zero-based. round 1 match 1's match placement is 0
  * @param $direction
  *   Either 'home' or 'away'
  * @return $place
  *   Match placement of the desired match, otherwise NULL 
  */
  function getNextMatch($place, $direction) {
    $next = $this->getNextMatchSlot($place, $direction);
    if ( $next === NULL ) return NULL;
    return $next['match'];
  }

 /**
  * Given a match place integer, returns the next match slot based on either 
  * 'home' or 'away' direction
  *
  * @param $match
  *   Match entity, since the tournament isn't handling this method to provide a 
  *   zero-index match place
  * @param $direction
  *   Either 'home' or 'away'
  * @return $place
  *   Slot placement of the desired match, otherwise NULL 
  */
  function getNextSlot($match, $direction) {
    $ids = array_flip($match->getTournament()->getMatchIds());
    $next = $this->getNextMatchSlot($ids[$match->entity_id], $direction);
    if ( $next === NULL ) return NULL;
    return $next['slot'] + 1;
  }
}