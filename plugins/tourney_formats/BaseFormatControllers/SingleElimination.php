<?php

/**
 * @file
 * Single elimination controller, new system.
 */

/**
 * A class defining how matches are created, and rendered for this style
 * tournament.
 */
class SingleEliminationController extends TourneyController {
  public $slots;

  /**
   * Constructor
   */
  public function __construct($numContestants, $tournament = NULL) {
    parent::__construct();
    // Set our contestants, and then calculate the slots necessary to fit them
    $this->numContestants = $numContestants;
    $this->slots = pow(2, ceil(log($this->numContestants, 2)));
    $this->tournament = $tournament;
  }

  /**
   * Options for this plugin.
   */
  public function optionsForm(&$form_state) {
    $this->getPluginOptions();
    $options = $this->pluginOptions;
    $plugin_options = array_key_exists(get_class($this), $options) ? $options[get_class($this)] : array();

    form_load_include($form_state, 'php', 'tourney', 'plugins/tourney_formats/BaseFormatControllers/SingleElimination');

    $form['players'] = array(
      '#type' => 'textfield',
      '#size' => 10,
      '#title' => t('Number of Contestants'),
      '#description' => t('Number of contestants that will be playing in this tournament.'),
      '#default_value' => array_key_exists('players', $plugin_options) ? $plugin_options['players'] : 2,
      '#disabled' => !empty($form_state['tourney']->id) ? TRUE : FALSE,
      '#element_validate' => array('singleelimination_players_validate'),
      '#attributes' => array('class' => array('edit-configure-round-names-url')),
      '#states' => array(
        'enabled' => array(
          ':input[name="format"]' => array('value' => get_class($this)),
        ),
      ),
    );
    $form['third_place'] = array(
      '#type' => 'checkbox',
      '#title' => t('Generate a third place match'),
      '#description' => t('By checking this option, a Consolation bracket will be created with one match to determine third place.'),
      '#default_value' => array_key_exists('third_place', $plugin_options)
        ? $plugin_options['third_place'] : -1,
      '#disabled' => !empty($form_state['tourney']->id) ? TRUE : FALSE,
    );

    return $form;
  }

  /**
   * Theme implementations specific to this plugin.
   */
  public static function theme($existing, $type, $theme, $path) {
    return parent::theme($existing, $type, $theme, $path) + array(
      'tourney_tournament_tree_node' => array(
        'variables' => array('plugin' => NULL, 'node' => NULL),
        'path' => $path . '/theme',
        'file' => 'preprocess_tournament_tree_node.inc',
        'template' => 'tourney-tournament-tree-node',
      ),
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
      $vars['matches'] = '';
      $node = $this->structure['tree'];

      // Set the matches variable.
      if (!empty($this->pluginOptions) && $this->pluginOptions[get_class($this)]['third_place']) {
        // Don't try to render the children of a third place match.
        unset($node['children']);

        // New tree should start from the second to last match.
        $match = $this->data['matches'][$node['id'] - 1];
        $last_node = $this->structureTreeNode($match);

        // Render the bracket out without the consolation bracket.
        $vars['matches'] .= theme('tourney_tournament_tree_node', array('plugin' => $this, 'node' => $last_node));
      }
      $vars['matches'] .= theme('tourney_tournament_tree_node', array('plugin' => $this, 'node' => $node));
    }
  }

  /**
   * This builds the data array for the plugin. The most important data structure
   * your plugin should implement in build() is the matches array. It is from
   * this array that matches are saved to the Drupal entity system using
   * TourneyController::saveMatches().
   */
  public function build() {
    parent::build();
    $this->buildBrackets();
    $this->buildMatches();
    $this->buildGames();

    // Check to see if we need to create a consolation bracket and matches.
    $this->getPluginOptions();
    $options = $this->pluginOptions;
    $plugin_options = array_key_exists(get_class($this), $options) ? $options[get_class($this)] : array();
    if (!empty($plugin_options) && $plugin_options['third_place']) {
      // Add the third place match to the data.
      $this->buildThirdPlace();
    }

    $this->data['contestants'] = array();

    // Calculate and set the match pathing
    $this->populatePositions();
    // Set in the seed positions
    $this->populateSeedPositions();
  }

  public function buildBrackets() {
    $this->data['brackets']['main'] = $this->buildBracket(array(
      'id' => 'main',
      'rounds' => log($this->slots, 2),
    ));
  }

  public function buildMatches() {
    $slots = $this->slots;
    $match = &drupal_static('match', 0);
    $round = &drupal_static('round', 0);

    // Calculate and iterate through rounds and their matches based on slots
    while (($slots /= 2) >= 1) {
      // Add current round information to the data array
      $this->data['rounds'][++$round] =
        $this->buildRound(array('id' => $round, 'bracket' => 'main', 'matches' => $slots));

      // Add in all matches and their information for this round
      foreach (range(1, $slots) as $roundMatch) {
        $this->data['matches'][++$match] =
          $this->buildMatch(array(
            'id' => $match,
            'round' => $round,
            'tourneyRound' => $round,
            'roundMatch' => (int) $roundMatch,
            'bracket' => 'main',
          ));
      }
    }
  }

  public function buildGames() {
    foreach ($this->data['matches'] as $id => &$match) {
      $this->data['games'][$id] = $this->buildGame(array(
        'id' => $id,
        'match' => $id,
        'game' => 1,
      ));
      $this->data['matches'][$id]['games'][] = $id;
    }
  }

  public function buildThirdPlace() {
    $match = &drupal_static('match', 0);

    $this->data['brackets']['consolation'] = $this->buildBracket(array('id' => 'consolation'));

    $this->data['matches'][++$match] = $this->buildMatch(array(
      'id' => $match,
      'round' => 1,
      'roundMatch' => 1,
      'bracket' => 'consolation',
    ));

    // Populate positions for third place match.
    $this->populatePositionsThirdPlace();
  }

  /**
   * Find and populate next/previous match for third place.
   */
  public function populatePositionsThirdPlace() {
    $count = count($this->data['matches']);
    $this->data['matches'][$count]['previousMatches'] = array($count - 3, $count - 2);

    // Set the next match for losers.
    foreach ($this->data['matches'][$count]['previousMatches'] as $mid) {
      $this->data['matches'][$mid]['nextMatch']['loser']['id'] = $count;
    }
  }


  /**
   * Find and populate next/previous match pathing on the matches data array for
   * each match.
   */
  public function populatePositions() {
    foreach ($this->data['matches'] as $id => &$match) {
      $nextMatch = &$this->find('matches', array(
        'round'       => $match['round'] + 1,
        'roundMatch'  => (int) ceil($match['roundMatch'] / 2)
      ), TRUE);
      if (!$nextMatch) continue;
      $slot = $match['id'] % 2 ? 1 : 2;
      $match['nextMatch']['winner'] = array('id' => $nextMatch['id'], 'slot' => $slot);
      $nextMatch['previousMatches'][$slot] = $id;
    }
  }

  /**
   * Calculate and fill seed data into matches. Also marks matches as byes if
   * the match is a bye.
   */
  public function populateSeedPositions() {
    $this->calculateSeeds();
    // Calculate the seed positions, then apply them to their matches while
    // also setting the bye boolean
    foreach ($this->data['seeds'] as $id => $seeds) {
      $match =& $this->data['matches'][$id];
      $match['seeds'] = $seeds;
      $match['bye'] = $seeds[2] === NULL;
      if ($match['bye'] && isset($match['nextMatch'])) {
        $slot = $match['id'] % 2 ? 1 : 2;
        $this->data['matches'][$match['nextMatch']['winner']['id']]['seeds'][$slot] = $seeds[1];
      }
    }
  }

  /**
   * Generate a structure based on data
   */
  public function structure($type = 'nested') {
    switch ($type) {
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
        // If this is a feeder match, don't build child that goes to other bracket.
        // if (array_key_exists('feeder', $match) && $match['bracket'] != $this->data['matches'][$child]['bracket']) {
        //   continue;
        // }
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
    while (($count = count($seeds)) < $this->slots) {
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
      foreach ($seeds as $seed) {
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
    for ($p = 0; $p < $count; $p += 2) {
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

  public function render($style = 'tree') {
    // Build our data structure
    $this->build();
    $this->structure($style);
    return theme('tourney_tournament_render', array('plugin' => $this));
  }

}

/**
 * Callback for #element_validate.
 *
 * @see SingleEliminationController::optionsForm()
 */
function singleelimination_players_validate($element, &$form_state) {
  $value = $element['#value'];
  if ($value < 2) {
    form_error($element, t('%name must be two or more.', array('%name' => $element['#title'])));
  }
}

