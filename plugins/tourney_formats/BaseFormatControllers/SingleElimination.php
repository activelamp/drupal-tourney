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
    parent::__construct();
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
    $match_num = 1;
    foreach (range(1, $this->rounds) as $round) {
      foreach (range(1, $this->slots / pow(2, $round)) as $match) {
        $matches[] = array('bracket' => 'top', 'round' => $round, 'match' => $match_num);
        $match_num++;
      }
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
   * Given a match place integer, returns the next match place based on either 
   * 'winner' or 'loser' direction. Calls the necessary tournament format plugin 
   * to get its result
   *
   * @param $match
   *   Match object to compare with the internal matchIds property to get its 
   *   match placement
   * @param $direction
   *   Either 'winner' or 'loser'
   * @return $match
   *   Match entity of the desired match, otherwise NULL 
   */
  public function getNextMatch($match, $direction = NULL) {
    $ids = array_flip($this->tournament->getMatchIds());
    $next = $this->calculateNextPosition($ids[$match->entity_id], $direction);
    if ( $next === NULL ) return NULL;
    $ids = array_flip($ids);
    if ( !array_key_exists((int)$next, $ids) ) return NULL;
    return entity_load_single('tourney_match', $ids[$next]);
  }
  
  /**
   * Given a match place integer, returns the next match place based on either 
   * 'winner' or 'loser' direction
   *
   * @param $place
   *   Match placement, zero-based. round 1 match 1's match placement is 0
   * @param $direction
   *   Either 'winner' or 'loser'
   * @return $place
   *   Match placement of the desired match, otherwise NULL 
   */
  protected function calculateNextPosition($place, $direction) {
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
      'bracket-top' => array(
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
      'bracket-top' => array(
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
    $static_match_num = &drupal_static('match_num_iterator', 1, TRUE);
    
    for ($slots_left = $slots; $slots_left >= 2; $slots_left /= 2) {
      $rounds['rounds']['round-' . $round_num] = $this->buildRound($slots_left, $round_num);
      $round_num++;
    }

    return $rounds;
  }

  protected function buildRound($slots, $round_num) {
    $static_match_num = &drupal_static('match_num_iterator', 1);
    
    // The plugin defines a title for rounds and hard codes the title it into 
    // the structure array for every round except the first one.
    if ($round_num == 1) {
      $round = array(
        'title' => array(
          'callback' => 'getRoundTitle',
          'args'     => array('round_num' => $round_num),
        ),
      );
    }
    else {
      // This logic could have been handled in the callback above. It is hard
      // coded here to provide a concrete example of both implementations.
      $round = array(
        'title' => t('Round ') . $round_num,
      );
    }
    
    for ($match_num = 1; $match_num <= ($slots / 2); ++$match_num) {
      $round['matches']['match-' . $static_match_num] = $this->buildMatch($slots, $round_num, $static_match_num);
      $static_match_num++;
    }

    return $round;
  }
  
  /**
   * Define the match callbacks implemented in this plugin.
   */
  protected function buildMatch($slots, $round_num, $match_num) {
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
   * Theme implementations to register with tourney module.
   * 
   * @see hook_theme().
   */
  public static function theme($existing, $type, $theme, $path) {
    return array(
      'tourney_single_tree' => array(
        'variables' => array('rounds' => NULL, 'small' => FALSE),
        'file' => 'single.inc', 
        'path' => $path . '/theme',
      ),
    );
  }
  
  /**
   * Renders the html for each round tournament
   * 
   * @param $tournament
   *   The tournament object
   * @param $matches
   *   An array of all the rounds and matches in a tournament.
   */
  public function render() {
    drupal_add_js($this->pluginInfo['path'] . '/theme/single.js');
    return theme('tourney_single_tree', array('tournament' => $this->tournament));
  }
  
  /**
   * Get the callbacks for this match from the structure of the plugin.
   * 
   * @param $match
   *   Match object.
   */
  public function getMatchCallbacks($match) {
    $match_location = $this->getMatchAddress($match);
    
    // Return just the portion of the structure array that we need. We know how 
    // this structure array is built, because this array was defined in this
    // plugin.
    return $this->tournament->data['bracket-' . $match_location['bracket']]['rounds']['round-' . $match_location['round_num']]['matches']['match-' . $match_location['match_num']];
  }
  
  /**
   * Get the round title
   */
  public function getRoundTitle($round_num) {
    if ($round_num == 1) {
      return 'Qualifying Round';
    }
    return 'Round ' . $round_num;
  }
  
  /**
   * Method to get contestants for this plugin.
   */
  public function getContestants() {
    // Take matches from only the first round, since those are the manually populated ones
    $matches = $this->tournament->data['bracket-top']['rounds']['round-1']['matches'];
    // Set up two arrays to fill for seed order
    $seed_1 = array();
    $seed_2 = array();
    foreach ($matches as $match_callbacks) {
      $match = $this->tournament->tourneyFormatPlugin
        ->$match_callbacks['current_match']['callback']($match_callbacks['current_match']['args']);
        
      $seed = 1;
      foreach ( $match->getContestants() as $eid => $contestant ) {
        $group = "seed_" . $seed++;
        ${$group}[$eid] = $contestant;
      }
    }
    // Reverse seed_2, so we return contestants as 1 2 3 4 8 7 6 5 
    $contestants = array_merge($seed_1, array_reverse($seed_2));
    return $contestants;
  }
}