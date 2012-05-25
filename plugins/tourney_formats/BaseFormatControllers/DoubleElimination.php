<?php

/**
 * @file
 * A Double Elimination class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */
class DoubleEliminationController extends SingleEliminationController implements TourneyControllerInterface {
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
   * Build the double elim bottom bracket
   *
   * @return $matches
   *   The bottom bracket matches array
   */
  protected function buildBottomBracket() {
    $matches = array();
    // Rounds is a certain number, 2, 4, 6, based on the contestants participating
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
    
    $tiebreaker = $this->buildMatch($static_match_num, array(), $bracket_info);
    $tiebreaker['next_match']['callback'] = 'getNextMatchTiebreaker';

    $rounds['rounds']['round-1']['matches']['match-' . $static_match_num++] = $tiebreaker;
    $rounds['rounds']['round-2']['matches']['match-' . $static_match_num] = $this->buildMatch($static_match_num++, array(), $bracket_info);

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
    
    $round_info = array(
      'title' => t('Round ') . $round_num,
    );
    
    // Bring the round number down to a unique number per group of two
    $er = ceil($round_num/2);
    // Matches is a certain number based on the round number and slots
    // The pattern is powers of two, counting down: 8 8 4 4 2 2 1 1
    $match_count = $slots / pow(2, $er+1);
    foreach ( range(1, $match_count) as $match ) {
      $round['matches']['match-' . $static_match_num] = $this->buildMatch($static_match_num, $round_info, $bracket_info);
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
    if ( $tiebreaker->getContestantIds() ) {
      if ( $tiebreaker->getWinner() ) return TRUE;
      return FALSE;
    }
    if ( $champion->getContestantIds() ) {
      if ( $champion->getWinner() ) return TRUE;
    }
    return FALSE;
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
   protected function calculateNextPosition($place, $direction = "winner") {
     // @todo find a better way to count matches
     $slots = $this->slots;
     // Set up our handy values
     $matches = $slots * 2 - 1;
     $top_matches = $slots - 1;
     $bottom_matches = $top_matches - 1;

     if ( $direction == 'winner' ) {
       // Top Bracket
       if ( $place < $top_matches ) {
         // Last match in the top bracket goes to the champion bracket
         if ( $place == $bottom_matches ) return $matches - 2;
         return parent::calculateNextPosition($place);
       }
       // Champion Bracket(s)
       elseif ( $place >= $matches - 2 ) {
         // Last match goes nowhere
         if ( $place == $matches - 1 ) return NULL;
         return $place + 1;
       }
       // Bottom Bracket
       else {
         // Get out series to find out how to adjust our place
         $series = $this->magicSeries($bottom_matches);
         return $place + $series[$place-$top_matches];
       }
     }
     elseif ( $direction == 'loser' ) {
       // Top Bracket
       if ( $place < $top_matches ) {
         // If we're in the first round of matches, it's rather simple
         if ( $place < $slots / 2 ) 
           return parent::calculateNextPosition($place) + ($bottom_matches/2);          
         // Otherwise, more magical math to determine placement
         return $place + $top_matches - pow(2, floor(log($top_matches - $place, 2)));
       }
     }
     return NULL;
   }

 /**
  * This is a special function that I could have just stored as a fixed array, but I wanted it to scale
  * It creates a special series of numbers that affect where loser bracket matches go
  *
  * @param $until
  *   @todo I should change this to /2 to begin with, but for now it's the full number of bottom matches
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
    // Remove the unnecessary last element in the series (which is the start of the next iteration) 
    while ( count($series) > $until ) 
      array_pop($series);
    // Reverse it so we work down
    return array_reverse($series);
  }

  public function getNextMatchTiebreaker($match, $direction = NULL) {
    $next = $this->getNextMatch($match, 'winner');
    if ( $next->getContestantIds() ) return $next;
    $matches = entity_load('tourney_match', $this->tournament->getMatchIds());
    $contestant_matches = array();
    foreach ( $matches as $entity_id => $tournament_match ) {
      if ( $entity_id == $match->entity_id ) continue;
      foreach ( $tournament_match->getContestants() as $eid => $contestant ) {
        if ( $eid == $match->getLoser() ) $contestant_matches[$tournament_match->eid] = $tournament_match;
      }
    }
    $first_loss = TRUE;
    foreach ( $contestant_matches as $eid => $contestant_match ) {
      if ( $contestant_match->getLoser() == $match->getLoser() ) {
        $first_loss = FALSE;
        break;
      }
    }
    if ( !$first_loss ) return NULL;

    return $next;
  }
}
