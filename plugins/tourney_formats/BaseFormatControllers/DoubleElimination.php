<?php

/**
 * @file
 * A Double Elimination class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class DoubleEliminationController extends SingleEliminationController implements TourneyControllerInterface {

  public $directions = array('winner', 'loser');
  /**
   * Theme implementations to register with tourney module.
   *
   * @see hook_theme().
   */
  public static function theme($existing, $type, $theme, $path) {
    return array(
      'tourney_double_tree' => array(
        'variables' => array('tournament' => NULL),
        'file' => 'double.inc',
        'path' => $path . '/theme',
      ),
      'tourney_dummy_rounds' => array(
        'variables' => array('count' => NULL, 'height' => NULL, 'small' => 1),
        'file' => 'double.inc',
        'path' => $path . '/theme',
      ),
      'tourney_dummy_match' => array(
        'variables' => array('count' => NULL, 'height' => NULL, 'small' => 1),
        'file' => 'double.inc',
        'path' => $path . '/theme',
      ),
      'tourney_double_top_bracket' => array(
        'variables' => array('rounds' => NULL),
        'file' => 'double.inc',
        'path' => $path . '/theme',
      ),
      'tourney_double_bottom_bracket' => array(
        'variables' => array('rounds' => NULL),
        'file' => 'double.inc',
        'path' => $path . '/theme',
      ),
      'tourney_double_champion_bracket' => array(
        'variables' => array('tournament' => NULL, 'rounds' => NULL),
        'file' => 'double.inc',
        'path' => $path . '/theme',
      ),
    );
  }
  
  /**
   * Default options form that provides the label widget that all fields
   * should have.
   */
  public function optionsForm(&$form_state) {
    return array();
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
    drupal_add_js($this->pluginInfo['path'] . '/theme/double.js');
    return theme('tourney_double_tree', array('tournament' => $this->tournament));
  }

  /**
   * Build the Double Elimination match list.
   *
   * @return $matches
   *   A flat array that can be used by TourneyController::saveMatches().
   */
  public function build() {
    $matches = array();
    $structure = $this->structure();
    foreach ($structure as $bracket_name => $bracket_info) {
      foreach ($bracket_info['rounds'] as $round_name => $round_info) {
        // It's possible in double elim that a bottom round has not matches.
        if (array_key_exists('matches', $round_info)) {
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
    }

    $this->matches = $matches;
    return $this->matches;
  }

  /**
   * Build the double elim bottom bracket
   *
   * @return $matches
   *   The bottom bracket matches array
   */
  protected function buildBottomBracket() {
    $matches = array();
    // Rounds is a certain number, 2, 4, 6, based on the contestants
    $rounds = (log($this->slots, 2) - 1) * 2;
    foreach (range(1, $rounds) as $round) {
      // Bring the round number down to a unique number per group of two
      $er = ceil($round/2);
      // Matches is a certain number based on the round number and slots
      // The pattern is powers of two, counting down: 8 8 4 4 2 2 1 1
      $m = $this->slots / pow(2, $er+1);
      foreach ( range(1, $m) as $match ) {
        $matches[] = array('bracket_name' => 'bracket-bottom', 'round_name' => $round, 'match_name' => $match);
      }
    }
    return $matches;
  }

  /**
   * Build bracket structure and logic.
   *
   * @return
   *   An array of bracket structure and logic.
   */
  protected function buildBracketByTree($slots) {
    $num_contestants = $this->tournament->players;
    $bw_num_matches = $this->slots - 1;
    $bl_num_matches = $num_contestants - 2;
    $bw_num_rounds = log($this->slots, 2);

    // @todo: account for byes
    $bl_num_rounds = ($bw_num_rounds - 1) * 2;

    $bracketw_info = array(
      'id'    => 'main',
      'name'  => 'bracket-top',
      'title' => t('Winners Bracket'),
    );
    $bracketl_info = array(
      'id'    => 'bottom',
      'name'  => 'bracket-bottom',
      'title' => t('Losers Bracket'),
    );
    $bracketc_info = array(
      'id'    => 'champion',
      'name'  => 'bracket-champion',
      'title' => t('Champion Bracket'),
    );

    // Championship
    $champ_match_num = $bw_num_matches + $bl_num_matches + 1;
    $champ_round_info = array(
      'id'    => 1,
      'name'  => 'round-1',
      'title' => 'Championship',
    );
    $tree = $this->buildMatch($champ_match_num, $champ_round_info, $bracketc_info);
    $champ_round_info['id'] = 2;
    $champ_round_info['name'] = 'round-2';
    $tree['sibling'] = $this->buildMatch($champ_match_num + 1, $champ_round_info, $bracketc_info);
    // Winner bracket
    $tree['children'][] = parent::buildBracketByTree($this->slots);
    // Loser bracket
    // @todo: remove later
    if ($num_contestants == 19) {
      $tree['children'][] = $this->buildLoserChildrenHard19(32, 7, 48, $bracketl_info);
    }
    else {
      $tree['children'][] = $this->buildLoserChildren($this->slots, $bl_num_rounds, $champ_match_num - 1, $bw_num_matches + 1, $bracketl_info);
    }

    return $tree;
  }

  protected function buildLoserChildren($slots, $round_num, $match_num, $match_num_start, $bracket_info, $iteration = 0) {
    $round_info = array(
      'id'    => $round_num,
      'name'  => 'round-' . $round_num,
      'title' => $this->getRoundTitle(array('round_num' => $round_num, 'bracket' => $bracket_info['id'])),
    );
    $tree = $this->buildMatch($match_num, $round_info, $bracket_info);

    if ($round_num > 1) {
      $matches_in_child_round = $this->getNumMatchesInLoserRound($slots, $round_num - 1);
      $children = ($round_num % 2) + 1;
      for ($i = 0; $i < $children; ++$i) {
        $child_match_num = ($match_num - $matches_in_child_round) + $i + ($children > 1 ? $iteration : 0);
        $tree['children'][] = $this->buildLoserChildren($slots, $round_num - 1, $child_match_num, $match_num_start, $bracket_info, $i + $iteration);
      }
    }

    return $tree;
  }

  protected function getNumMatchesInLoserRound($slots, $round_num) {
    $num_rounds = log($slots, 2) - 1;
    $matches_in_rounds = array();
    for ($i = 0; $i < $num_rounds; ++$i) {
      $matches_in_rounds[] = pow(2, $i);
      $matches_in_rounds[] = pow(2, $i);
    }
    $matches_in_rounds = array_reverse($matches_in_rounds);
    return $matches_in_rounds[$round_num - 1];
  }

  protected function buildLoserChildrenHard19($slots, $round_num, $match_num, $bracket_info, $iteration = 0) {
    $matches_in_rounds = array(4,4,4,2,2,1,1);
    $num_children = array(1,1,1,2,1,2,1);
    $round_info = array(
      'id'    => $round_num,
      'name'  => 'round-' . $round_num,
      'title' => $this->getRoundTitle(array('round_num' => $round_num)),
    );
    $tree = $this->buildMatch($match_num, $round_info, $bracket_info);

    if ($round_num > 1) {
      $matches_in_child_round = $matches_in_rounds[$round_num - 2];
      $children = $num_children[$round_num - 1];
      for ($i = 0; $i < $children; ++$i) {
        $child_match_num = ($match_num - $matches_in_child_round) + $i + ($children > 1 ? $iteration : 0);
        $child_match_num = $match_num == 35 ? 32 : $child_match_num;
        if ($round_num - 1 == 1 && $match_num == 36) {
          continue;
        }
        $tree['children'][] = $this->buildLoserChildrenHard19($slots, $round_num - 1, $child_match_num, $bracket_info, $i + $iteration);
      }
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
  public function buildBracketByRounds($slots) {
    $top_bracket_info = array(
      'id'    => 'main',
      'name'  => 'bracket-top',
      'title' => t('Winners Bracket'),
    );

    $bottom_bracket_info = array(
      'id'    => 'bottom',
      'name'  => 'bracket-bottom',
      'title' => t('Losers Bracket'),
    );

    $champion_bracket_info = array(
      'id'    => 'champion',
      'name'  => 'bracket-champion',
      'title' => t('Champion Bracket'),
    );
    return array(
      'bracket-top' => $top_bracket_info + $this->buildRounds($slots, $top_bracket_info),
      'bracket-bottom' => $bottom_bracket_info + $this->buildBottomRounds($slots, $bottom_bracket_info),
      'bracket-champion' => $champion_bracket_info + $this->buildChampionRounds($slots, $champion_bracket_info),
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
  protected function buildChampionRounds($slots, $bracket_info) {
    $rounds    = array();
    $static_match_num = &drupal_static('match_num_iterator', 1);

    $tiebreaker = $this->buildMatch($static_match_num, array('id' => 1, 'round-1', 'title' => 'Championship'), $bracket_info);
    $tiebreaker['is_won']['callback'] = 'tiebreakerIsWon';

    $rounds['rounds']['round-1']['matches']['match-' . $static_match_num++] = $tiebreaker;
    $rounds['rounds']['round-2']['matches']['match-' . $static_match_num] = $this->buildMatch($static_match_num++, array('id' => 2, 'round-2', 'title' => 'Tiebreaker'), $bracket_info);

    return $rounds;
  }

  /**
   * Build rounds.
   *
   * @param $slots
   *   The number of slots in the first round.
   * @return
   *   The rounds array completely built out.
   */
  protected function buildBottomRounds($slots, $bracket_info) {
    $rounds    = array();
    $round_num = 1;
    $static_match_num = &drupal_static('match_num_iterator', 1);

    $num_rounds = (log($this->slots, 2) - 1) * 2;
    foreach (range(1, $num_rounds) as $round_num) {
      $rounds['rounds']['round-' . $round_num] = $this->buildBottomRound($slots, $round_num, $bracket_info);
    }
    return $rounds;
  }

  protected function buildBottomRound($slots, $round_num, $bracket_info) {
    $static_match_num = &drupal_static('match_num_iterator', 1);
    $round = array();
    $round_info = array(
      'id'    => $round_num,
      'name'  => 'round-' . $round_num,
      'title' => $this->getRoundTitle(array('round_num' => $round_num, 'bracket' => $bracket_info['id'])),
    );

    $match_count = $this->getBottomMatchCount($slots, $round_num);
    for ($i = 1; $i <= $match_count; $i++) {
      $round['matches']['match-' . $static_match_num] = $this->buildMatch($static_match_num, $round_info, $bracket_info);
      $round['matches']['match-' . $static_match_num]['is_won']['callback'] = 'bottomMatchIsWon';
      $static_match_num++;
    }

    return $round + $round_info;
  }

  /**
   * Overrides SingleElimination::isFinished().
   *
   * If the top two ranking contestants are not tied in their number of wins
   * then we do not require the final match to be finished.
   *
   * @see SingleEliminationController::isFinished().
   */
  public function isFinished($tournament) {
    $ids = $tournament->getMatchIds();
    $tiebreaker = entity_load_single('tourney_match', array_pop($ids));
    $champion   = entity_load_single('tourney_match', array_pop($ids));
    $tiebreaker->contestantIds = NULL;
    if ( $tiebreaker->getContestantIds() ) {
      if ( $tiebreaker->getWinner() ) return TRUE;
      return FALSE;
    }
    $champion->contestantIds = NULL;
    if ( $champion->getContestantIds() ) {
      if ( $champion->getWinner() ) return TRUE;
    }
    return FALSE;
  }

  /**
    * Given a match place integer, returns the next match place based on
    * either 'winner' or 'loser' direction
    *
    * @param $place
    *   Match placement, zero-based. round 1 match 1's match placement is 0
    * @param $direction
    *   Either 'winner' or 'loser'
    * @return $place
    *   Match placement of the desired match, otherwise NULL
    */
  protected function calculateNextPosition($place, $direction = "winner") {
    // @todo find a better way to count matches
    $slots = $this->slots;
    // Set up our handy values
    $matches = $slots * 2 - 1;
    $top_matches = $slots - 1;
    $bottom_matches = $top_matches - 1;

    // Champion Bracket
    if ( $place >= $matches - 2 ) {
      // Last match goes nowhere
      if ( $place == $matches - 1 ) return NULL;
      return $place + 1;
    }

    if ( $direction == 'winner' ) {
     // Top Bracket
     if ( $place < $top_matches ) {
       // Last match in the top bracket goes to the champion bracket
       if ( $place == $top_matches - 1 ) return $matches - 2;
       return parent::calculateNextPosition($place);
     }
     // Bottom Bracket
     else {
       // Get out series to find out how to adjust our place
       $series = $this->magicSeries($top_matches - 1);
       return $place + $series[$place-$top_matches];
     }
    }
    elseif ( $direction == 'loser' ) {
      // Top Bracket
      if ( $place < $top_matches ) {
        // If we're in the first round of matches, it's rather simple
        $bottom_bracket = $this->tournament->data['bracket-bottom'];
        if ( $place < $slots / 2 ) {
          $adj = 0;
          // Some special adjustment comes in to bump the matches if the first
          // round of the bottom bracket has no matches.
          if ( !array_key_exists('matches', $bottom_bracket['rounds']['round-1']) ) {
            $adj = 1;
          }
          return parent::calculateNextPosition($place) + ($bottom_matches/2) + $adj;
        }
        // Otherwise, more magical math to determine placement
        $rev_round = floor(log($top_matches - $place, 2)) ;
        // Special adjustments come in on certain rounds of matches that
        // generally flips them around as such:
        //
        // 1, 2, 3, 4, 5, 6, 7, 8
        //          \/
        // 5, 6, 7, 8, 1, 2, 3, 4
        //
        // and on the special occasions with byes, it can go:
        //
        // 6, 5, 8, 7, 2, 1, 4, 3
        //
        if ( ( $rev_round - count($this->structure['bracket-top']['rounds']) ) % 2 == 0 ) {
          $round_matches = pow(2, $rev_round);
          $first_match = $top_matches - $round_matches * 2 + 1;
          $this_match = $place - $first_match;
          $half_matches = $round_matches / 2;
          $adj = 0;
          // Same special adjustment from the first round comes into play here in the second round
          if ( $place < $slots * 0.75 && !array_key_exists('matches', $bottom_bracket['rounds']['round-1']) ) {
            $adj = $this_match % 2 ? -1 : 1;
          }
          return $place + $top_matches - $round_matches + ( ( $this_match < $half_matches ) ? $half_matches : -$half_matches ) + $adj;
        }
        return $place + $top_matches - pow(2, floor(log($top_matches - $place, 2)));
      }
    }
    return NULL;
  }

  /**
   * Calculate number of players playing in bottom round.
   *
   * @param $slots
   *   The number of slots in the tournament
   * @param $round_num
   *   The round number integer to calculate.
   * @param $use_byes
   *   Allow byes to remove some rounds
   * @return
   *   The number of matches in the first round of bottom round.
   */
  protected function getBottomMatchCount($slots, $round_num, $use_byes = TRUE) {
    static $byes = NULL;

    $num_matches = $this->calculateBottomRound($slots, $round_num);

    if (!$use_byes) {
      return $num_matches;
    }

    // The first round we need half the number of players from top round
    if ($round_num <= 2) {
      if (is_null($byes)) {
        $seed_positions = $this->calculateSeedPositions($this->slots);
        $byes = 0;
        foreach ($seed_positions as $seed) {
          if (in_array(NULL, $seed)) {
            $byes++;
          }
        }
      }
      $deductor = min($num_matches, $byes);
      $byes -= $deductor;
      $num_matches -= $deductor;
    }

    return $num_matches;
  }

  public function calculateBottomRound($slots, $round_num) {
    // Bring the round number down to a unique number per group of two
    $er = ceil($round_num/2);
    // Matches is a certain number based on the round number and slots
    // The pattern is powers of two, counting down: 8 8 4 4 2 2 1 1
    return $slots / pow(2, $er+1);
  }

 /**
  * This is a special function that I could have just stored as a fixed array,
  * but I wanted it to scale. It creates a special series of numbers that
  * affect where loser bracket matches go
  *
  * @param $until
  *   @todo I should change this to /2 to begin with, but for now it's the
  *   full number of bottom matches
  * @return $series
  *   Array of numbers
  */
  private function magicSeries($until) {
    $series = array();
    $i = 0;
    // We're working to 8 if until is 16, 4 if until is 8
    while ( $i < $until / 2 ) {
      // Add in this next double entry of numbers
      $series[] = ++$i;
      $series[] = $i;
      // If it's a power of two, throw in that many numbers extra
      if ( ($i & ($i - 1)) == 0 )
        foreach ( range(1, $i) as $n ) $series[] = $i;
    }
    // Remove the unnecessary last element in the series (which is the start
    // of the next iteration)
    while ( count($series) > $until )
      array_pop($series);
    // Reverse it so we work down
    return array_reverse($series);
  }

  /**
   * Fill matches in the bottom bracket that are produced as a result of a bye.
   * Dummy matches are created in the same order that players are seeded. This
   * method fills up an entire round to contain the number of matches without
   * byes, fills the spaces with the string 'dummy'.
   *
   * @param $round_info
   *   The round information from the plugin.
   */
  public function fillMatches(&$round_info) {

    // Byes only affect the first two rounds of the bottom bracket.
    if ($round_info['id'] > 2) {
      return;
    }

    if (!array_key_exists('matches', $round_info) || !$round_info['matches']) {
      return;
    }

    $seed_positions = $this->calculateSeedPositions($this->slots);
    $num_matches = count($round_info['matches']);
    // Retrieve the number of matches there is supposed to be w/o byes.
    $num_matches_total = $this->calculateBottomRound($this->slots, $round_info['id']);

    // The number of matches to render dummy matches
    $dummy_match_count = $num_matches_total - $num_matches;

    $new_matches = array();
    for ($i = 0; $i < count($seed_positions); $i += 2) {
      if ($seed_positions[$i][0] <= $dummy_match_count) {
        $new_matches[] = 'dummy';
      }
      else {
        $new_matches[] = array_shift($round_info['matches']);
      }
    }
    $round_info['matches'] = $new_matches;
  }

  /**
   * Emulated an n**2 contestant match, filling in placeholders to take the
   * place of matches that don't exist in the bottom bracket because of byes.
   *
   */
  public function getFullMatchIds() {
    // Store the data for quick access on subsequent calls
    static $fullMatchIds = '';
    if ( !$fullMatchIds ) {
      $new_ids = array();
      $ids = $this->tournament->getMatchIds();
      // Get all the dummy'd up rounds for the bottom bracket.
      $rounds = array();
      foreach ( $this->tournament->data['bracket-bottom']['rounds'] as $r => $round ) {
        $rounds[$r] = $this->fillMatches($round);
        if ( !$rounds[$r] ) $rounds[$r] = array_key_exists('matches', $round) ? $round['matches'] : NULL;
      }
      if ( $rounds['round-1'] == NULL ) {
        while ( count($rounds['round-1']) < count($rounds['round-2']) ) {
          $rounds['round-1'][] = 'dummy';
        }
      }
      $matches = array();
      // Go through our rounds and replace all 'dummy's with negative numbers.
      // In this way they're unique numbers can can be array flipped, and also
      // distinct from the (positive) normal match numbers
      $x = -1;
      foreach ( $rounds as $round ) {
        foreach ( $round as $match ) {
          if ( $match !== 'dummy' ) {
            $matches[] = $match['id'];
          }
          else {
            $matches[] = $x--;
          }
        }
      }
      // Go through all our ids and throw the dummies in when necessary
      foreach ( $ids as $n => $id ) {
        if ( $n < $this->slots - 1 ) {
          $new_ids[] = array_shift($ids);
        }
        else {
          while ( $matches && ($i = array_shift($matches)) < $n ) {
            $new_ids[] = $i;
          }
          $new_ids[] = array_shift($ids);
        }
      }
      $fullMatchIds = $new_ids;
    }
    return $fullMatchIds;
  }

  /**
   * Given a match place integer, returns the next match place based on either
   * 'winner' or 'loser' direction. Calls the necessary tournament format
   * plugin to get its result
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
    if ( $direction == 'loser' && array_key_exists('bye', $match->matchInfo) && $match->matchInfo['bye'] == TRUE )
      return NULL;
    $ids = array_flip($this->getFullMatchIds());
    $next = $this->calculateNextPosition($ids[$match->entity_id], $direction);
    if ( $next === NULL ) return NULL;
    $ids = array_flip($ids);
    while ( $ids[$next] < 0 ) {
      $next = $this->calculateNextPosition($next, 'winner');
    }
    if ( !array_key_exists((int)$next, $ids) ) return NULL;
    return entity_load_single('tourney_match', $ids[$next]);
  }

  /**
   * Callback when a match is won, handles moving contestants
   *
   * @param $match
   *   Match object to win
   */
  public function matchIsWon($match) {
    if (!$match->isFinished() && !array_key_exists('bye', $match->matchInfo)) return;

    // clear winners to ensure new winners are loaded
    $match->clearWinner();
    $winner = $match->getWinnerEntity();
    $loser  = $match->getLoserEntity();

    // odd matches go to top slot, even to bottom
    $slot = $match->matchInfo['id'] % 2 ? 1 : 2;
    if ($match->nextMatch()) {
      $match->nextMatch()->addContestant($winner, $slot);
    }
    if ($match->nextMatch('loser')) {
      // Check to see if next match feeds both contestants from main bracket.
      $pm = $match->nextMatch('loser')->previousMatches();
      $feedboth = $pm[0]->matchInfo['bracket']['id'] == 'main'
        && $pm[1]->matchInfo['bracket']['id'] == 'main' ? TRUE : FALSE;

      // if we're not in the first round and not feeding both contestants, send
      // the loser to the top slot.
      if ($match->matchInfo['round']['id'] > 1 && !$feedboth) {
        $slot = 1;
      }
      $match->nextMatch('loser')->addContestant($loser, $slot);
    }
  }

  /**
   * Callback when a match is won, handles moving contestants
   * Override for bottom round matches
   *
   * @param $match
   *   Match object to win
   */
  public function bottomMatchIsWon($match) {
    if (!$match->isFinished()) return;
    // set up some values to determine our placement in the bottom bracket
    $tournament = $match->getTournament();
    $bottomBracket = $tournament->data['bracket-bottom']['rounds'];
    $roundId = $match->matchInfo['round']['id'];

    $thisRound = $bottomBracket['round-' . $roundId];
    $nextRound = NULL;
    if ( array_key_exists('round-' . ($roundId + 1), $bottomBracket) )
      $nextRound = $bottomBracket['round-' . ($roundId + 1)];

    $match->clearWinner();
    $winner = $match->getWinnerEntity();
    if ( !$winner ) return;
    // odd bottom, even top
    // (because first match is an even number instead of odd like top)
    $slot = $match->matchInfo['id'] % 2 ? 2 : 1;
    if ( $nextRound && count($thisRound['matches']) <= count($nextRound['matches']) ) {
      // however, if we're feeding into a round that pulls losers from the top
      // bracket we'll throw our winner into the bottom slot
      $slot = 2;
    }
    if ( $match->nextMatch() )
      $match->nextMatch()->addContestant($winner, $slot);
  }

  /**
   * Callback when a match is won, handles moving contestants
   * Override for tiebreaker match
   *
   * @param $match
   *   Match object to win
   */
  public function tiebreakerIsWon($match) {
    $match->clearWinner();
    $winner = $match->getWinnerEntity();
    if ( !$winner ) return;
    // check the previous match (last match in the top bracket)
    // to see if the winner of that was the winner of this
    // because if that's the case, we're not going anywhere
    // with our contestants
    $previousMatches = $match->previousMatches();
    $top    = $previousMatches[0];
    if ( $top->getWinner() == $winner->eid ) return;
    // otherwise we've got to head to the last match to break this tie
    return $this->matchIsWon($match);
  }
}
