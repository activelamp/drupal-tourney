<?php

/**
 * @file
 * Double elimination controller, new system.
 */

/**
 * A class defining how matches are created, and rendered for this style
 * tournament.
 */
class DoubleEliminationController extends SingleEliminationController {
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
            'tourneyRound' => $round,
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
          'tourneyRound' => $round,
          'roundMatch' => 1,
          'bracket' => 'champion',
      ));
    }
  }
  
  /**
   * Fix the next match previous match slot numbers for bottom bracket. The
   * logic for these positions is calculated in a parent class.  Rather than
   * change the parent class to accommodate this child class, just go back thru
   * the array and remove the previous position added by parent class.
   * 
   * SpecialDoubleElimination::populateLoserPositions() handles putting the
   * correct slot number in the data array..
   */
  public function fixFeeders() {
    foreach ($this->data['matches'] as $id => &$match) {
      if ($match['bracket'] == 'loser') {
        if (!empty($this->data['matches'][$id]['feeder']) 
          && $this->data['matches'][$id]['feeder'] == TRUE) {
        
          unset($this->data['matches'][$id]['previousMatches'][1]);
        }
      }
    }
  }
  
  /**
   * Find and populate next/previous match pathing on the matches data array for
   * each match.
   */
  public function populatePositions() {
    parent::populatePositions();
    // Populates the loser positions.
    $this->populateLoserPositions();
    // Fix the next match previous match slot numbers for bottom bracket that 
    // were populated by parent class.
    $this->fixFeeders();
  }
  
  
  /**
   * Populates nextMatch and previousMatches array on the match data.
   * 
   * @todo: Clean this function up... Way too complicated.
   */
  public function populateLoserPositions() {
    // Go through all the matches
    $count = count($this->data['matches']);
    $top_matches = $this->slots - 1;
    
    foreach ($this->data['matches'] as $id => &$match) {
      // Set the paths for the main bracket
      if ($match['bracket'] == 'main') {
        // Calculate all the next loser positions in the top bracket.
        $next = $this->calculateNextPosition($match, 'loser');
        $match['nextMatch']['loser'] = $next;
        if (!array_key_exists('previousMatches', $this->data['matches'][$next['id']])
          || $top_matches != $id) {
          $this->data['matches'][$next['id']]['previousMatches'][$next['slot']] = $id;
        }        
        
        // Set the winner path for the last match of the main bracket
        if ($top_matches == $id) {
          $next_id = count($this->data['matches']) - 1;
          $match['nextMatch']['winner']['id'] = $next_id;
          $match['nextMatch']['winner']['slot'] = 1;
          $this->data['matches'][$next_id]['previousMatches'][1] = $id;
        }
        
        // If a previous match is a feeder, set a flag.
        if ($this->data['matches'][$next['id']]['bracket'] == 'loser') {
          $this->data['matches'][$next['id']]['feeder'] = TRUE;
        }
      }
      elseif ($match['bracket'] == 'loser') {
        // Calculate all the next loser positions in the bottom bracket.
        $next = $this->calculateNextPosition($match, 'winner');
        $match['nextMatch']['winner'] = $next;
        
        // If the next round is a feeder or the next match bracket is different
        // than the current bracket, slot goes to position 2.
        if (!empty($this->data['matches'][$next['id']]['feeder']) 
          && $this->data['matches'][$next['id']]['feeder'] == TRUE
          || $match['bracket'] != $this->data['matches'][$next['id']]['bracket']) {
            
          $match['nextMatch']['winner']['slot'] = 2;
          $this->data['matches'][$next['id']]['previousMatches'][2] = $id;
        }
        
        // Don't overwrite the previousMatch I just set above for the last match
        // of the top bracket
        $champion_match = count($this->data['matches']) - 1;
        if ($champion_match != $next['id']) {
          $this->data['matches'][$next['id']]['previousMatches'][$next['slot']] = $id;
        }
      }
      elseif ($match['bracket'] == 'champion') {
        $next = count($this->data['matches']);
        if ($next - 1 == $id) {
          $match['nextMatch']['winner'] = array('id' => $next, 'slot' => 1);
          $match['nextMatch']['loser'] = array('id' => $next, 'slot' => 2);
        }
        // The very last match of the tournament
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
    * @param $match_info
    *   The match info data array created by a plugin.
    * @param $direction
    *   Either 'winner' or 'loser'
    * @return array
    *   A keyed array for the id of next match and slot as the keys.
    */
  protected function calculateNextPosition($match_info, $direction = "loser") {
    // @todo find a better way to count matches
    $slots = $this->slots;
    // Set up our handy values
    $matches = $slots * 2 - 1;
    $top_matches = $slots - 1;
    $bottom_matches = $top_matches - 1;
    $place = $match_info['id'];
    $slot = $match_info['roundMatch'] % 2 ? 1 : 2;
    
    // Champion Bracket
    if ($place >= $matches - 2) {
      // Last match goes nowhere
      if ($place == $matches - 1) {
        return NULL;
      }
      return array(
        'id' => $place + 1,
        'slot' => $slot,
      );
    }
    
    if ($direction == 'winner') {
     // Top Bracket
     if ($place < $top_matches) {
       // Last match in the top bracket goes to the champion bracket
       if ($place == $top_matches) {
         return array(
           'id' => $matches - 1,
           'slot' => $slot,
         );
       }
       $next = parent::calculateNextPosition($match_info);
       return array(
        'id' => $next['id'],
        'slot' => $slot,
       );
     }
     // Bottom Bracket
     else {
       // Get out series to find out how to adjust our place
       $series = $this->magicSeries($bottom_matches);
       return array(
        'id' => $place + $series[$place - $top_matches],
        'slot' => $slot,
       );
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
          $next = parent::calculateNextPosition($this->data['matches'][$place]);
          return array(
            'id' => $next['id'] + ($bottom_matches / 2),
            'slot' => $slot,
          );
        }
        
        // Losers always feed into bottom round in slot 1.
        $slot = 1;
        
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
        
        // Get the number of matches in this round
        $find_matches = $this->find('matches', array(
          'round' => $this->data['matches'][$place]['round'],
          'bracket' => $this->data['matches'][$place]['bracket'],
        ));
        $round_matches = count($find_matches);
        $half = $round_matches / 2;
        
        // Figure out if we need to reverse the round, set a boolean flag if
        // we're in an even round.
        $reverse_round = $this->data['matches'][$place]['round'] % 2 ? FALSE : TRUE;
        
        if ($reverse_round) {
          // Since this round is a reverse round we need to put the first half 
          // of matches in the second half of the round
          if ($this->data['matches'][$place]['roundMatch'] <= $half) {
            $adj = $half;
            return array(
              'id' => floor($place + $top_matches - $round_matches + $adj),
              'slot' => $slot,
            );
          }
          // And then move what is supposed to be in the second half to the 
          // first half of the round.
          else {
            $adj = -$half;
            return array(
              'id' => ceil($place + $top_matches - $round_matches + $adj),
              'slot' => $slot,
            );
          }
        }
        // Non reverse rounds just need to cut by half the number of matches in 
        // the round plus 1 (@todo: figure out why this works and document it.).
        else {
          $adj = floor(-1 * ($half / 2)) + 1;
        }
        
        return array(
          'id' => $place + $top_matches - $round_matches + $adj,
          'slot' => $slot,
        );
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
        $next = $match['nextMatch']['winner']['id'];
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
            if (array_key_exists('loser', $this->data['matches'][$child]['nextMatch']) && $this->data['matches'][$child]['nextMatch']['loser']['id'] == $match['id']) {
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
