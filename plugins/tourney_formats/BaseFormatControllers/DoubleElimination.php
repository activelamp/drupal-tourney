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
   public function optionsForm(&$form_state) {
     $this->getPluginOptions();
     $options = $this->pluginOptions;
     $plugin_options = array_key_exists(get_class($this), $options) ? $options[get_class($this)] : array();

     form_load_include($form_state, 'php', 'tourney', 'plugins/tourney_formats/BaseFormatControllers/DoubleElimination');

     $form['players'] = array(
       '#type' => 'textfield',
       '#size' => 10,
       '#title' => t('Number of Contestants'),
       '#description' => t('Number of contestants that will be playing in this tournament.'),
       '#default_value' => array_key_exists('players', $plugin_options) ? $plugin_options['players'] : 3,
       '#disabled' => !empty($form_state['tourney']->id) ? TRUE : FALSE,
       '#element_validate' => array('doubleelimination_players_validate'),
     );

     return $form;
   }

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

      if (!isset($this->pluginOptions['show_byes']) || $this->pluginOptions['show_byes'] == FALSE)
        $this->removeByes($node);
      
      // New tree should start from the second to last match.
      // $match = $this->data['matches'][15];
      // $last_node = $this->structureTreeNode($match);
      
      // Render the consolation bracket out.
      $vars['matches'] .= theme('tourney_tournament_tree_node', array('plugin' => $this, 'node' => $node));
    }
  }

  public function removeByes(&$node) {
    if (!isset($node['children'])) return;
    foreach ($node['children'] as $id => $child) {
      if (isset($child['bye']) && $child['bye'] == TRUE) unset($node['children'][$id]);
    }
    if (count($node['children'])) {
      foreach ($node['children'] as &$child) $this->removeByes($child);
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
      // Bring the round number down to a unique number per group of two
      $round_group = ceil($round_num / 2);
      
      // Matches is a certain number based on the round number and slots
      // The pattern is powers of two, counting down: 8 8 4 4 2 2 1 1

      $num_matches = $this->slots / pow(2, $round_group + 1);
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round_num, 'bracket' => 'loser', 'matches' => $num_matches));

      foreach (range(1, $num_matches) as $roundMatch) {
        $this->data['matches'][++$match] = $this->buildMatch(array(
          'id' => $match,
          'round' => (int) $round_num,
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
      $this->data['rounds'][++$round] = $this->buildRound(array(
        'id' => $round_num,
        'bracket' => 'champion'
      ));
      $this->data['matches'][++$match] = $this->buildMatch(array(
        'id' => $match,
        'round' => $round_num,
        'tourneyRound' => $round,
        'roundMatch' => 1,
        'bracket' => 'champion',
        'sibling' => $round_num == 2 ? TRUE : FALSE
      ));
    }
  }
  
  /**
   * Find and populate next/previous match pathing on the matches data array for
   * each match.
   */
  public function populatePositions() {
    $mainCount  = count($this->find('matches', array('bracket' => 'main')));
    $loserCount = count($this->find('matches', array('bracket' => 'loser')));
    foreach ($this->data['matches'] as $id => &$match) {
      if ($match['id'] == count($this->data['matches'])) continue;
      $properties  = array();
      $properties['round'] = $match['round'] + 1;
      switch ($match['bracket']) {
        case 'main':
          $slot = $match['id'] % 2 ? 1 : 2;
          $properties['bracket']    = 'main';
          $properties['roundMatch'] = (int) ceil($match['roundMatch'] / 2);
          break;
        case 'loser':
          $properties['bracket']  = 'loser';
          if ($match['round'] % 2) {
            $slot = 2;
            $properties['roundMatch'] = $match['roundMatch'];
          }
          else {
            $slot = $match['id'] % 2 ? 2 : 1;
            $properties['roundMatch'] = (int) ceil($match['roundMatch'] / 2);
          }
          break;
        case 'champion':
          if ($match['round'] == 1) {
            $properties['bracket']  = 'champion';
            $properties['round']    = 2;
            $slot = 1;
          }
          else {
            $properties = array();
          }
          break;
      }
      if ($match['id'] == $mainCount || $match['id'] == $mainCount + $loserCount) {
        $slot = $match['bracket'] == 'main' ? 1 : 2;
        $properties['bracket']  = 'champion';
        $properties['round']    = 1;
      }

      $nextMatch = &$this->find('matches', $properties, TRUE);
      if (!$nextMatch) continue;

      $match['nextMatch']['winner'] = array('id' => $nextMatch['id'], 'slot' => $slot);
      $nextMatch['previousMatches'][$slot] = $id;
      unset($nextMatch);
    }
    // Populates the loser positions.
    $this->populateLoserPositions();
  }

  /**
   * Populates nextMatch and previousMatches array on the match data that has
   * already been built from parent class.
   * 
   * @todo: Clean this function up... Way too complicated.
   */
  public function populateLoserPositions() {
    $oddRounds = count($this->data['rounds']) % 2;
    foreach ($this->data['matches'] as $id => &$match) {
      if ($match['bracket'] !== 'main') continue;
      $properties = array();
      $properties['bracket'] = 'loser';
      $roundMatches = $this->data['rounds'][$match['round']]['matches'];
      $roundMatchesHalf = ceil($roundMatches / 2);
      if ($match['round'] == 1) {
        if ($oddRounds) {
          $target = ceil($match['roundMatch'] / 2);
        } 
        else {
          $target = (ceil($match['roundMatch'] / 2) + ceil($roundMatchesHalf / 2) - 1) % $roundMatchesHalf + 1;          
        }
        $slot = $match['roundMatch'] % 2 ? 2 : 1;
        $properties['round']      = 1;
        $properties['roundMatch'] = (int) $target;
      }
      else {
        $slot = 1;
        $properties['round'] = ($match['round'] - 1) * 2;
        if (($match['round'] + $oddRounds) % 2) {
          $target = ($match['roundMatch'] + $roundMatchesHalf - 1) % $roundMatches + 1;
        }
        else {
          $target = $match['roundMatch'];
        }
        $properties['roundMatch'] = (int) $target;
      }
      if (!$properties) continue;
      $nextMatch = &$this->find('matches', $properties, TRUE);
      if (!$nextMatch) continue;

      $match['nextMatch']['loser'] = array('id' => $nextMatch['id'], 'slot' => $slot);
      $nextMatch['previousMatches'][$slot] = $id;
    }
    $championMatch = &$this->find('matches', array('bracket' => 'champion', 'round' => 1), TRUE);
    $championMatch['nextMatch']['loser'] = array(
      'id' => $championMatch['id'] + 1,
      'slot' => 2,
    );
  }
  
  /**
   * Look at each match in the data array and count how many matches are in the
   * given round passed in.
   * 
   * @param $bracket
   *   The machine name of the bracket.
   * @param $round
   *   The machine name of the round.
   */
  protected function matchesInRound($bracket, $round) {
    $count = 0;
    foreach ($this->data['matches'] as $match) {
      if ($match['bracket'] == $bracket && $match['round'] == $round) {
        $count++;
      }
    }
    return $count;
  }
  
  /**
   * Calculate and fill seed data into matches. Also marks matches as byes if
   * the match is a bye.
   */
  public function populateSeedPositions() {
    parent::populateSeedPositions();

    $matches = $this->find('matches', array('bracket' => 'main'));
    foreach ($matches as $match) {
      if (isset($match['bye']) && $match['bye'] == TRUE) {
        $this->data['matches'][$match['nextMatch']['loser']['id']]['bye'] = TRUE;
      }
    }
    
    $this->populateLoserByes();


    // Update byes to remove themselves from their path
    $byes = $this->find('matches', array('bracket' => 'loser', 'bye' => TRUE));
    foreach ($byes as $match) {
      $winner = $match['nextMatch']['winner'];
      foreach ($match['previousMatches'] as $k => $v) {
        $prev = &$this->data['matches'][$v];
        if (isset($prev['bye']) && $prev['bye'] == TRUE) continue; 
        $prev['nextMatch']['loser'] = array('id' => $winner['id'], 'slot' => $winner['slot']);
      }
    }

    $this->adjustLoserByes();
  }
  

  public function adjustLoserByes() {
    $matches = &$this->find('matches', array('bracket' => 'loser', 'round' => 3));
    foreach ($matches as &$match) {
      $byes = 0;
      foreach ($match['previousMatches'] as $id) {
        if ($this->data['matches'][$id]['bye'] == TRUE) $byes++;
      }
      if ($byes !== 1) continue;
      $previousLoser  = &$this->data['matches'][$match['previousMatches'][1]];
      $previousBye    = &$this->data['matches'][$match['previousMatches'][2]];
      $previousWinner = &$this->data['matches'][$previousBye['previousMatches'][1]];
      $previousWinner['nextMatch']['loser']['slot'] = 1;
      $previousLoser['nextMatch']['winner']['slot'] = 2;
    } 
  }

  /**
   * Goes through all the matches in the first round of the loser bracket and
   * marks matches as byes in the data array of the match.
   */
  protected function populateLoserByes() {
    $matches = $this->find('matches', array('bracket' => 'loser', 'round' => 2));
    foreach ($matches as &$match) {
      if (!isset($match['previousMatches'][2])) continue;
      $previousMatch = $this->data['matches'][$match['previousMatches'][2]];
      $byes = 0;
      foreach ($previousMatch['previousMatches'] as $id) {
        $prev = $this->data['matches'][$id];
        if (isset($prev['bye']) && $prev['bye'] == TRUE) $byes++;
      }
      $match['bye'] = FALSE;
      if ($byes == 2) $match['bye'] = TRUE;
    }
  }

  public function structureTreeNode($match) {
   // if ($match['bracket'] != 'loser') return parent::structureTreeNode($match);
    $node = $match;
    if (isset($match['previousMatches'])) {
      foreach (array_unique($match['previousMatches']) as $child) {
        if ($match['bracket'] == 'loser' && $match['bracket'] !== $this->data['matches'][$child]['bracket']) continue;
        if ($match['id'] == $child) continue;
        $node['children'][] = $this->structureTreeNode($this->data['matches'][$child]);
      }
    }
    return $node;
  }

}

/**
 * Callback for #element_validate.
 *
 * @see DoubleEliminationController::optionsForm()
 */
function doubleelimination_players_validate($element, &$form_state) {
  $value = $element['#value'];
  if ($value < 3) {
    form_error($element, t('%name must be three or more.', array('%name' => $element['#title'])));
  }
}

