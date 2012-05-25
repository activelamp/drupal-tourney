<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */

class RoundRobinController extends TourneyController implements TourneyControllerInterface {

  protected $tournament;
  protected $slots;
  protected $contestants;
  protected $matches;

  /**
   * Constructor
   */
  public function __construct(TourneyTournament $tournament) {
    $this->tournament = $tournament;
    
    $this->contestants = $tournament->players;
    // Ensure we have an even number of slots
    $slots = $this->contestants % 2 ? $this->contestants + 1 : $this->contestants;
    $this->slots = $slots;
    
    parent::__construct();
  }
  
  /**
   * Theme implementations to register with tourney module.
   * 
   * @see hook_theme().
   */
  public static function theme($existing, $type, $theme, $path) {
    return array(
      'tourney_roundrobin_standings' => array(
        'variables' => array('tournament' => NULL),
        'file' => 'roundrobin.inc', 
        'path' => $path . '/theme',
      ),
      'tourney_roundrobin' => array(
        'variables' => array('tournament' => NULL),
        'file' => 'roundrobin.inc', 
        'path' => $path . '/theme',
      ),
    );
  }
  
  /**
   * Build an array with tournament structure data.
   *
   * @return $structure
   *   An array of the tournament structure and meta data.
   */
  public function structure($type = 'rounds') {
    return $this->structure = $this->buildBracketByRounds($this->slots);
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
      'bracket-roundrobin' => array(
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
    $total_rounds = $slots - 1;
    $rounds = array();
    $static_match_num = &drupal_static('match_num_iterator', 1, TRUE);
    
    for ($round_num = 1; $round_num <= $total_rounds; $round_num++) {
      $rounds['rounds']['round-' . $round_num] = $this->buildRound($slots, $round_num);
    }

    return $rounds;
  }
  
  protected function buildRound($slots, $round_num) {
    $static_match_num = &drupal_static('match_num_iterator', 1);
    
    $round = array(
      'title' => t('Round ') . $round_num,
    );

    for ($match_num = 1; $match_num <= ($slots / 2); $match_num++) {
      $round['matches']['match-' . $static_match_num] = $this->buildMatch($static_match_num);
      $static_match_num++;
    }

    return $round;
  }
  
  /**
   * Define the match callbacks implemented in this plugin.
   */
  protected function buildMatch($match_num) {
    return array(
      'current_match' => array(
        'callback' => 'getMatchByName',
        'args' => array(
          'match_name' => 'match-' . $match_num,
        ),
      ),
      'previous_match' => array(
        'callback' => 'getPreviousMatch',
      ),
      'next_match' => array(
        'callback' => 'getNextMatch',
        'args' => array(
          'direction' => 'winner',
        ),
      ),
    );
  }
  
  /**
   * Renders the html for each round of a round robin tournament
   * 
   * @param $tournament
   *   The tournament object
   * @param $matches
   *   An array of all the rounds and matches in a tournament.
   */
  public function render() {
    drupal_add_js($this->pluginInfo['path'] . '/theme/roundrobin.js');
    return theme('tourney_roundrobin', array('tournament' => $this->tournament));
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
    $structure = $this->structure();
    // Number of rounds is (n - 1), n being contestants
    $rounds = $this->slots - 1;
    foreach ($structure as $bracket_name => $bracket_info) {
      foreach ($bracket_info['rounds'] as $round_name => $round_info) {
        foreach ($round_info['matches'] as $match_name => $match_info) {
          $matches[] = array(
            'bracket_name' => $bracket_name, 
            'round_name' => $round_name, 
            'match_name' => $match_name,
            'match_info' => $match_info,
          );
        }
      }
    }
    $this->matches = $matches;
    return $this->matches;
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

    return $tournament;
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
  *   Either '0' or '1'
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
  * @param $match
  *   Match object.
  * @param $slot
  *   Either '0' or '1'
  * @return $place
  *   Match placement of the desired match, otherwise NULL 
  */
  function getNextMatch($match, $slot) {
    $ids = array_flip($this->tournament->getMatchIds());
    $next = $this->getNextMatchSlot($ids[$match->entity_id], $slot);
    if ( $next === NULL ) return NULL;
    
    $ids = array_flip($ids);
    if (!array_key_exists((int)$next['match'], $ids)) return NULL;
    return entity_load_single('tourney_match', $ids[$next['match']]);
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
  *   Slot placement of the desired match, otherwise NULL (1 based counting)
  */
  function getNextSlot($match, $direction) {
    $ids = array_flip($match->getTournament()->getMatchIds());
    $next = $this->getNextMatchSlot($ids[$match->entity_id], $direction);
    if ( $next === NULL ) return NULL;
    return $next['slot'] + 1;
  }
  
  /**
   * Get the callbacks for this match from the structure of the plugin.
   */
  public function getMatchCallbacks($match) {
    $match_location = $this->getMatchAddress($match);
    // Return just the portion of the structure array that we need. We know how 
    // this structure array is built, because this array was defined in this
    // plugin.
    // 
    // @todo: For some reason in round robin, this data variable isn't populated
    // on instantiation.
    $this->tournament->data = $this->tournament->tourneyFormatPlugin->structure();
    
    return $this->tournament->data['bracket-' . $match_location['bracket']]['rounds']['round-' . $match_location['round_num']]['matches']['match-' . $match_location['match_num']];
  }

  /**
   * Runs related functions when a match is won, called from rules.
   */
  public function winMatch($match) {
    $this->populateMatches($match);
  }

/**
 * Recursive function that sets players in a RoundRobin tournament in all their
 * matches once the first round has been setup.
 *
 * @param $match (object)
 *   The match the game belongs to
 */
  public function populateMatches($match) {
    foreach ($match->getContestants() as $contestant) {
      $next = $match->nextMatch($contestant->slot);
      
      if ( $next === NULL ) continue;
      $slot = $match->getTournament()->tourneyFormatPlugin->getNextSlot($match, $contestant->slot);

      // If the slot is already taken, don't do anything
      if ( $next->getContestant($slot) ) continue;
      $next->addContestant($contestant, $slot);
      // Recurse
      $this->populateMatches($next);
    }
  }
}