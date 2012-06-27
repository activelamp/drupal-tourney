<?php

/**
 * @file
 * Single elimination controller, new system.
 */

class SpecialEliminationController extends TourneyController {
  public $slots;

  /**
   * Constructor
   */
  public function __construct($numContestants, $tournament = NULL) {
    // Set our contestants, and then calculate the slots necessary to fit them 
    $this->numContestants = $numContestants;  
    $this->slots = pow(2, ceil(log($this->numContestants, 2)));
    $this->tournament = $tournament;
  }
  
  /**
   * Default options form that provides the label widget that all fields
   * should have.
   */
  public function optionsForm(&$form_state) {
    $form['third_place'] = array(
      '#type' => 'checkbox',
      '#title' => t('Generate a third place match'),
      '#description' => t('By checking this option, a Consolation bracket will be created with one match to determine third place.'),
      '#default_value' => '',
    );
    
    return $form;
  }
  
  /**
   * Theme implementations specific to this plugin.
   */
  public static function theme($existing, $type, $theme, $path) {
    return parent::theme($existing, $type, $theme, $path) + array(
      'tourney_tournament_tree_node' => array(
        'variables' => array('node'),
        'path' => $path . '/theme',
        'file' => 'preprocess_tournament_tree_node.inc',
        'template' => 'tourney-tournament-tree-node',
      )
    );
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
    }
  }

  /**
   * Data generator
   */
  public function build() {    
    $slots = $this->slots;
    $match = 0;
    $round = 0;
    
    // Add current bracket information to the data array
    $this->data['brackets']['main'] = $this->buildBracket(array('id' => 'main'));
    
    // Calculate and iterate through rounds and their matches based on slots
    while (($slots /= 2) >= 1) {
      // Add current round information to the data array
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round));

      // Add in all matches and their information for this round
      foreach (range(1, $slots) as $roundMatch) {
        $this->data['matches'][++$match] = 
          $this->buildMatch(array(
            'id' => $match,
            'round' => $round,
            'roundMatch' => (int) $roundMatch,
            'bracket' => 'main',
          ));
      }
    }
    
    // Check to see if we need to create a consolation bracket and matches.
    $plugin_options = $this->tournament->get(__CLASS__, array());
    if (!empty($plugin_options) && $plugin_options['third_place']) {
      $this->data['brackets']['consolation'] = $this->buildBracket(array('id' => 'consolation'));
      
      $this->data['matches'][++$match] = $this->buildMatch(array(
        'id' => $match,
        'round' => 1,
        'roundMatch' => 1,
        'bracket' => 'consolation',
      ));
    }
    
    foreach ($this->data['matches'] as $id => &$match) {
      $this->data['games'][$id] = $this->buildGame(array(
        'id' => $id,
        'match' => $id,
        'game' => 1, 
      ));
      $this->data['matches'][$id]['games'][] = $id;
    }
    
    $this->data['contestants'] = array();     
    
    // Calculate and set the match pathing
    $this->populatePositions();
    // Set in the seed positions
    $this->populateSeedPositions();
  }


  /**
   * Find and populate next/previous match pathing on the matches data array for
   * each match.
   */
  public function populatePositions() {
    // Go through all the matches
    $count = count($this->data['matches']);
    foreach ($this->data['matches'] as $id => &$match) {
      if ($id == $count) {
        continue;
      }
      // Find next match by filtering through matches with in the next round
      // and those with a halved round match number
      // Example:
      //   Round 3, Match 5
      //  Next match is:
      //    Round 4 [3+1], Match 3 [ceil(5/2)]
      $next = &$this->find('matches', array(
        'round' => $match['round'] + 1,
        'roundMatch' => (int) ceil($match['roundMatch'] / 2),
      ), TRUE);

      // $index = ( $this->slots / 2 ) + floor(($match->match-1) / 2)+1;
      // $next = $this->data['matches'][$index];

      // If find()'s returned a result, set it.
      if ($next) {
        $match['nextMatch']['winner'] = $next['id'];
        $next['previousMatches'][] = $match['id'];
      } 
    }
  }

  /**
   * Calculate and fill seed data into matches
   */
  public function populateSeedPositions() {
    $this->calculateSeeds();
    // Calculate the seed positions, then apply them to their matches while
    // also setting the bye boolean
    foreach ( $this->data['seeds'] as $id => $seeds ) {
      $match =& $this->data['matches'][$id];
      $match['seeds'] = $seeds;
      $match['bye']   = $seeds[2] === NULL;
      if ( $match['bye'] && isset($match['nextMatch']) ) {
        $slot = $match['id'] % 2 ? 1 : 2;
        $this->data['matches'][$match['nextMatch']['winner']]['seeds'][$slot] = $seeds[1];
      }
    }
  }

  /**
   * Generate a structure based on data
   */
  public function structure($type = 'nested') {
    switch ( $type ) {
      case 'nested':
        $this->structure['nested'] = $this->structureNested();
        break;
      case 'tree':
        $this->structure['tree'] = $this->structureTree();
        break;
    }
    return $this->structure[$type];
  }

  public function structureNested() {
    $structure = array();
    // Loop through our rounds and set up each one
    foreach ($this->data['rounds'] as $round) {
      $structure[$round['id']] = $round + array('matches' => array());
    }
    // Loop through our matches and add each one to its related round
    foreach ($this->data['matches'] as $match) {
      $structure['round-' . $match['round']]['matches'][$match['id']] = $match;
    }
    return $structure;
  }

  public function structureTree() {
    $match = end($this->data['matches']);
    return $this->structureTreeNode($match);
  }

  public function structureTreeNode($match) {
    $node = $match;
    if (isset($match['previousMatches'])) {
      foreach ($match['previousMatches'] as $child) {
        $node['children'][] = $this->structureTreeNode($this->data['matches'][$child]);
      }
    }
    return $node;
  }

  /**
   * Calculate contestant starting positions
   */
  public function calculateSeeds() {
    // Set up the first seed position
    $seeds = array(1);
    // Setting a count variable lets us speed up execution by not calling it
    // several times each iteration
    $count = 0;
    // Keep generating the series until we've met our number of slots
    while ( ($count = count($seeds)) < $this->slots ) {
      $new_seeds = array();
      // For every current seed number, we'll add in one after it that
      // matches the current from the end of the new count.
      // Example:
      // (1, 2)
      //   The new series will have 4 elements, which is the current * 2
      // 4 - 1 = 3, however because we're 1-based, add 1: 4
      //  (1, 4)
      // 4 - 2 = 2 + 1 = 3
      //  (2, 3)
      // New series:
      // (1, 4, 2, 3)
      foreach ( $seeds as $seed ) {
        $new_seeds[] = $seed;
        $new_seeds[] = ($count * 2 + 1) - $seed;
      }
      // Set these changes to be iterated through again if necessary
      $seeds = $new_seeds;
    }
    // Now that we've generated a full list of positions, fill them out into
    // a series of matches with two positions per.
    // Loop through each two of them, and fill out all the seeds as long as
    // they're within the number of participating contestants.
    $positions = array();
    for ( $p = 0; $p < $count; $p += 2 ) {
      $a = $seeds[$p];
      $b = $seeds[$p+1];
      $positions[] = array(1 => $a, 2 => $b <= $this->numContestants ? $b : NULL);
    }
    // This logic changed our zero-based array to one-based so our match
    // ids will line up
    array_unshift($positions, NULL);
    unset($positions[0]);
    $this->data['seeds'] = $positions;
  }

  public function render() {
    // Build our data structure
    $this->build();
    $this->structure('tree');
    return theme('tourney_tournament_render', array('format_plugin' => $this, 'theme' => 'tourney_tournament_tree_node'));
  }
}