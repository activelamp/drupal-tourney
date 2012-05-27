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
  protected $seedPositions = NULL;

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
   * Build the Single Elimination match list.
   *
   * @return $matches
   *   A flat array that can be used by TourneyController::saveMatches().
   */
  public function build() {
    $matches = array();
    $structure = $this->structure();
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
   * Calculate the starting seed positions.
   *
   * @return $matches
   *   An array of match pairs with seed numbers, or NULL for bye slots.
   */
  public function calculateSeedPositions($num_contestants) {    
    if (is_null($this->seedPositions)) {
      // Initialize a the first match in the first round matches.
      $first_round_matches = array(array(1, 2));
      $num_first_round_matches = pow(2, $this->rounds) / 2;

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
              // Create a bye match.
              $new_matches[] = array($seed_position, NULL);
            }
          }
        }
        $first_round_matches = $new_matches;
      }
      $this->seedPositions = $first_round_matches;
    }
    return $this->seedPositions;
  }
  
  /**
   * Determine if byes in the previous round create manual slots in this round
   */
  protected function setByeManuals($matches, $round = 2) {
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

  public function getPreviousMatches($match) {
    $ids = array_flip($this->tournament->getMatchIds());
    $prevs = $this->calculatePreviousPositions($ids[$match->id]);
    if ( $prevs === NULL ) return array(NULL, NULL);
    $ids = array_flip($ids);
    return array_values(entity_load('tourney_match', array($ids[$prevs[0]], $ids[$prevs[1]])));
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
  protected function calculateNextPosition($place, $direction = 'winner') {
    if ( $direction == 'loser' ) return NULL;
    $matches = $this->slots - 1;
    // If it's the last match, it doesn't go anywhere
    if ( $place == $matches - 1 ) return NULL;
    // Otherwise some math!
    return ( ($matches + 1) / 2 ) + floor($place / 2);
  }

  protected function calculatePreviousPositions($place) {
    $matches = $this->slots - 1;
    if ( $place < $this->slots / 2 ) return NULL;
    $first = ( $place * 2 ) - $this->slots;
    return array($first, $first + 1);
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
    $num_matches = $slots - 1;
    $num_rounds = log($slots, 2);
    $bracket_info = array(
      'id'    => 'main',
      'name'  => 'bracket-main',
      'title' => t('Main Bracket'),
    );

    if (!is_int($num_rounds)) {
      // @todo: Acocunt for byes.
    }

    return $this->buildChildren($slots, $num_rounds, $num_matches, $bracket_info);
  }

  protected function buildChildren($slots, $round_num, $match_num, $bracket_info) {
    $round_info = array(
      'id'    => $round_num,
      'name'  => 'round-' . $round_num,
      'title' => $this->getRoundTitle(array('round_num' => $round_num)),
    );
    $tree = $this->buildMatch($match_num, $round_info, $bracket_info);

    if ($round_num > 1) {
      $child_match_num = ($match_num - ($slots / 2)) * 2;
      $tree['children'][] = $this->buildChildren($slots, $round_num - 1, $child_match_num - 1, $bracket_info);
      $tree['children'][] = $this->buildChildren($slots, $round_num - 1, $child_match_num, $bracket_info);
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
    $bracket_info = array(
      'id'    => 'main',
      'name'  => 'bracket-main',
      'title' => t('Main Bracket'),
    );
    return array(
      'bracket-top' => $bracket_info + $this->buildRounds($slots, $bracket_info),
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
  protected function buildRounds($slots, $bracket_info) {
    $rounds = array();
    $round_num = 1;
    $static_match_num = &drupal_static('match_num_iterator', 1, TRUE);

    for ($slots_left = $slots; $slots_left >= 2; $slots_left /= 2) {
      $round_info = array(
        'id'    => $round_num,
        'name'  => 'round-' . $round_num,
        'title' => $this->getRoundTitle(array('round_num' => $round_num)),
      );
      $rounds['rounds']['round-' . $round_num] = $this->buildRound($slots_left, $round_info, $bracket_info);
      $round_num++;
    }

    return $rounds;
  }

  protected function buildRound($slots, $round_info, $bracket_info) {
    $static_match_num = &drupal_static('match_num_iterator', 1);
    // Copy round info to the round level of the array for convenience.
    $round = $round_info;

    for ($match_num = 1; $match_num <= ($slots / 2); ++$match_num) {
      $round['matches']['match-' . $static_match_num] = $this->buildMatch($static_match_num, $round_info, $bracket_info);
      $static_match_num++;
    }

    return $round;
  }

  /**
   * Build out match data.
   */
  protected function buildMatch($match_num, $round, $bracket) {
    $match = array();
    $seed_positions = $this->calculateSeedPositions($this->tournament->players);
    $match_position = $match_num - 1;
    
    if (!empty($seed_positions[$match_position])) {
      $match += array('seed_position' => array(
        $seed_positions[$match_position][0],
        $seed_positions[$match_position][1],
      ));
      // Set a bye flag if this is a bye match
      $match += array_search(NULL, $seed_positions[$match_position])
        ? array('bye' => TRUE) : array();
    }
    
    $match += array(
      'id' => $match_num,
      'name' => 'match-' . $match_num,
      'round' => $round,
      'bracket' => $bracket,
      'current_match' => array(
        'callback' => 'getMatchByName',
        'args' => array(
          'match_name' => 'match-' . $match_num,
        ),
      ),
      'previous_matches' => array(
        'callback' => 'getPreviousMatches',
      ),
      'next_match' => array(
        'callback' => 'getNextMatch',
        'args' => array(
          'direction' => 'winner',
        ),
      ),
    );
    
    return $match;
  }

  /**
   * Theme implementations to register with tourney module.
   *
   * @see hook_theme().
   */
  public static function theme($existing, $type, $theme, $path) {
    return array(
      'tourney_single_tree' => array(
        'variables' => array('tournament' => NULL),
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
   * Get the round title
   */
  public function getRoundTitle($vars) {
    $round_num = $vars['round_num'];
    if ($round_num == 1) {
      return 'Qualifying Round';
    }
    return 'Round ' . $round_num;
  }

  /**
   * Runs related functions when a match is won, called from rules.
   */
  public function winMatch($match) {
    $match->cleanGames();
    $match->moveContestants();
    $match->determineWinner();
  }
}