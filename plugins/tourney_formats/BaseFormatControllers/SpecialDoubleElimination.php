<?php

/**
 * @file
 * Double elimination controller, new system.
 */

/**
 * A class defining how matches are created, and rendered for this style
 * tournament.
 */
class SpecialDoubleEliminationController extends SingleEliminationController {
  /**
   * Options for this plugin.
   */
  public function optionsForm(&$form_state) {}
  
  /**
   * Theme implementations specific to this plugin.
   */
  public static function theme($existing, $type, $theme, $path) {
    // Get the plugin info for the class we are inheriting.
    $parent_info = TourneyController::getPluginInfo('SingleEliminationController');
    return parent::theme($existing, $type, $theme, $parent_info['path']);
  }
  
  /**
   * Preprocess variables for the template passed in.
   * 
   * @param $template
   *   The name of the template that is being preprocessed.
   * @param $vars
   *   The vars array to add variables to.
   */
  public function preprocess($template, &$vars) {
    if ($template == 'tourney-tournament-render') {
      $vars['classes_array'][] = 'tourney-tournament-tree';
      $vars['matches'] = '';
      $node = $this->structure['tree'];
      
      // New tree should start from the second to last match.
      // $match = $this->data['matches'][15];
      // $last_node = $this->structureTreeNode($match);
      
      // Render the consolation bracket out.
      $vars['matches'] .= theme('tourney_tournament_tree_node', array('plugin' => $this, 'node' => $node));
    }
  }
  
  public function buildBrackets() {
    parent::buildBrackets();
    $this->data['brackets']['loser'] = $this->buildBracket(array(
      'id' => 'loser',
      'rounds' => (log($this->slots, 2) - 1) * 2,
    ));
    $this->data['brackets']['champion'] = $this->buildBracket(array(
      'id' => 'champion',
      'rounds' => 2,
    ));
  }
  
  public function buildMatches() {
    parent::buildMatches();
    $this->buildBottomMatches();
    $this->buildChampionMatches();
  }
  
  public function buildBottomMatches() {
    $match = &drupal_static('match', 0);
    $round = &drupal_static('round', 0);
    
    // Rounds is a certain number, 2, 4, 6, based on the contestants
    $num_rounds = (log($this->slots, 2) - 1) * 2;
    foreach (range(1, $num_rounds) as $round_num) {
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round_num, 'bracket' => 'loser'));
        
      // Bring the round number down to a unique number per group of two
      $round_group = ceil($round_num / 2);
      
      // Matches is a certain number based on the round number and slots
      // The pattern is powers of two, counting down: 8 8 4 4 2 2 1 1
      $num_matches = $this->slots / pow(2, $round_group + 1);
      foreach (range(1, $num_matches) as $roundMatch) {
        $this->data['matches'][++$match] = 
          $this->buildMatch(array(
            'id' => $match,
            'round' => $round_num,
            'roundMatch' => (int) $roundMatch,
            'bracket' => 'loser',
        ));
      }
    }
  }
  
  public function buildChampionMatches() {
    $match = &drupal_static('match', 0);
    $round = &drupal_static('round', 0);
    
    foreach (array(1, 2) as $round_num) {
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round_num, 'bracket' => 'champion'));
      
      $this->data['matches'][++$match] = 
        $this->buildMatch(array(
          'id' => $match,
          'round' => $round_num,
          'roundMatch' => 1,
          'bracket' => 'champion',
      ));
    }
  }
  
  /**
   * Find and populate next/previous match pathing on the matches data array for
   * each match.
   */
  public function populatePositions() {
    parent::populatePositions();
    $this->populateLoserPositions();
  }
  
  public function populateLoserPositions() {
    // Go through all the matches
    $count = count($this->data['matches']);
    foreach ($this->data['matches'] as $id => &$match) {
      // Set the paths for the main bracket
      if ($match['bracket'] == 'main') {
        // Calculate all the next loser positions in the top bracket.
        $next = $this->calculateNextPosition($id, 'loser');
        $match['nextMatch']['loser'] = $next;
        if (!array_key_exists('previousMatches', $this->data['matches'][$next])
          || count($this->data['matches'][$next]['previousMatches']) < 2) {
          $this->data['matches'][$next]['previousMatches'][] = $id;
        }        
        
        // Set the winner path for the last match of the main bracket
        $top_matches = $this->slots - 1;
        if ($top_matches == $id) {
          $next = count($this->data['matches']) - 1;
          $match['nextMatch']['winner'] = $next;
          array_unshift($this->data['matches'][$next]['previousMatches'], $id);
        }
        
        // If a previous match is a feeder, set a flag.
        if ($this->data['matches'][$next]['bracket'] == 'loser') {
          $this->data['matches'][$next]['feeder'] = TRUE;
        }
      }
      elseif ($match['bracket'] == 'loser') {
        // Calculate all the next loser positions in the bottom bracket.
        $next = $this->calculateNextPosition($id, 'winner');
        $match['nextMatch']['winner'] = $next;
        
        $pms = $this->data['matches'][$next]['previousMatches'];
        if (count($pms) < 2 && $pms[0] != $id) {
          $this->data['matches'][$next]['previousMatches'][] = $id;
        }
      }
      elseif ($match['bracket'] == 'champion') {
        $next = count($this->data['matches']);
        if ($next - 1 == $id) {
          $match['nextMatch']['winner'] = $next;
          $match['nextMatch']['loser'] = $next;
        }
        elseif ($next == $id) {
          $this->data['matches'][$id]['previousMatches'][] = $id - 1;
        }
      }
    }
  }
  
  public function render() {
    return parent::render();
  }
  
  /**
    * Given a match place integer, returns the next match place based on
    * either 'winner' or 'loser' direction
    *
    * @param $place
    *   Match placement, one-based. round 1 match 1's match placement is 1
    * @param $direction
    *   Either 'winner' or 'loser'
    * @return $place
    *   Match placement of the desired match, otherwise NULL
    */
  protected function calculateNextPosition($place, $direction = "loser") {
    // @todo find a better way to count matches
    $slots = $this->slots;
    // Set up our handy values
    $matches = $slots * 2 - 1;
    $top_matches = $slots - 1;
    $bottom_matches = $top_matches - 1;

    // Champion Bracket
    if ($place >= $matches - 2) {
      // Last match goes nowhere
      if ($place == $matches - 1) {
        return NULL;
      }
      return $place + 1;
    }
    
    if ($direction == 'winner') {
     // Top Bracket
     if ($place < $top_matches) {
       // Last match in the top bracket goes to the champion bracket
       if ($place == $top_matches) {
         return $matches - 1;
       }
       return parent::calculateNextPosition($place);
     }
     // Bottom Bracket
     else {
       // Get out series to find out how to adjust our place
       $series = $this->magicSeries($bottom_matches);
       return $place + $series[$place - $top_matches];
     }
    }
    elseif ($direction == 'loser') {
      // Top Bracket
      if ($place <= $top_matches) {
        // If we're calculating next position for a match that is coming from 
        // the first round in the top bracket, that math is simply to find the
        // the next match winner position and then add half the number of
        // bottom matches to that position, to find the bottom loser position.
        if ($this->data['matches'][$place]['round'] == 1) {
          return parent::calculateNextPosition($place) + ($bottom_matches / 2);
        }
        
        // Otherwise, more magical math to determine placement. Every even round
        // in the top bracket we need to flip the matches so that the top half of 
        // matches go to the bottom as such:
        //
        // 1, 2, 3, 4, 5, 6, 7, 8
        //          \/
        // 5, 6, 7, 8, 1, 2, 3, 4
        //
        // and on the special occasions with byes, it can go:
        //
        // 6, 5, 8, 7, 2, 1, 4, 3
        //
        
        // Figure out if we need to reverse the round, set a boolean flag if
        // we're in an even round.
        $reverse_round = $this->data['matches'][$place]['round'] % 2 ? FALSE : TRUE;
        
        // Get the number of matches in this round
        $match_info = $this->find('matches', array('round' => $this->data['matches'][$place]['round']));
        $round_matches = count($match_info);
        $half = $round_matches / 2;
        if ($reverse_round) {
          // Since this round is a reverse round we need to put the first half 
          // of matches in the second half of the round
          if ($this->data['matches'][$place]['roundMatch'] <= $half) {
            $adj = $half + 1;
            return floor($place + $top_matches - $round_matches + $adj);
          }
          // And then move what is supposed to be in the second half to the 
          // first half of the round.
          else {
            $adj = -$half + 1;
            return ceil($place + $top_matches - $round_matches + $adj);
          }
        }
        // Non reverse rounds just need to cut by half the number of matches in 
        // the round plus 1 (@todo: figure out why this works and document it.).
        else {
          $adj = floor(-1 * ($half / 2)) + 1;
        }
        // Last match in top bracket adjust forward by one.
        // @todo: Another adjustment that I made that works, need to figure out
        //   why and document it.
        if ($place == $top_matches) {
          $adj = -1;
        }
        return $place + $top_matches - $round_matches + $adj;
      }
    }
    return NULL;
  }
  
  /**
   * Calculate and fill seed data into matches. Also marks matches as byes if
   * the match is a bye.
   */
  public function populateSeedPositions() {
    parent::populateSeedPositions();
    $this->populateLoserByes();
  }
  
  /**
   * Goes through all the matches in the first round of the loser bracket and
   * marks matches as byes in the data array of the match.
   */
  protected function populateLoserByes() {
    foreach ($this->data['matches'] as &$match) {
      if ($match['bracket'] == 'loser' && $match['round'] == 1) {
        $next = $match['nextMatch']['winner'];
        foreach ($match['previousMatches'] as $child) {
          // If this match has both contestants as a bye mark the next match as
          // a bye too.
          if (!empty($match['bye']) && $match['bye'] == TRUE && array_key_exists('bye', $this->data['matches'][$child]) && $this->data['matches'][$child]['bye'] == TRUE) {
            $this->data['matches'][$next]['bye'] = TRUE;
          }
          
          // Set this match to a bye if one contestant has a bye in previous
          // match, or is already marked as a bye.
          if (array_key_exists('bye', $this->data['matches'][$child]) && $this->data['matches'][$child]['bye'] == TRUE 
            || array_key_exists('bye', $match) && $match['bye']) {
            $match['bye'] = TRUE;
            
            $this->data['matches'][$child]['nextMatch']['loser'] = $match['nextMatch']['winner'];
          }
        }
      }
      if ($match['bracket'] == 'loser' && $match['round'] == 2) {
        if (array_key_exists('bye', $match) && $match['bye'] == TRUE) {
          foreach ($match['previousMatches'] as $child) {
            if (array_key_exists('loser', $this->data['matches'][$child]['nextMatch']) && $this->data['matches'][$child]['nextMatch']['loser'] == $match['id']) {
              $this->data['matches'][$child]['nextMatch']['loser'] = $match['nextMatch']['winner'];
            }
          }
        }
      }
    }
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
    while ($i < $until / 2) {
      // Add in this next double entry of numbers
      $series[] = ++$i;
      $series[] = $i;
      // If it's a power of two, throw in that many numbers extra
      if (($i & ($i - 1)) == 0) {
        foreach (range(1, $i) as $n) {
          $series[] = $i;
        }
      }
    }
    // Remove the unnecessary last element in the series (which is the start
    // of the next iteration)
    while (count($series) > $until) {
      array_pop($series);
    }
    
    // Reverse it so we work down, and make the array 1-based
    return array_combine(range(1, count($series)), array_reverse($series));
  }
}
