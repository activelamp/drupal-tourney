<?php

/**
 * @file
 * Regular season controller, new system.
 */

/**
 * A class defining how matches are created, and rendered for this style
 * tournament.
 */
class RegularSeasonController extends TourneyController {
  /** Number of contestants required per round to fill all matches for the
      round. */
  public $slots;
  /** Total number of matches if each team only plays each other once. */
  public $matches_total;
  /** Total number of rounds if each team only plays each other once. */
  public $rounds_total;
  /** Number of matches per round. */
  public $matches_per_round;
  /** Number of times team A must play team B. */
  public $rounds_multiplier = 1;

  /**
   * Constructor
   */
  public function __construct($numContestants, $tournament = NULL) {
    parent::__construct();
    // Set our contestants, and then calculate the slots necessary to fit them
    $this->numContestants = $numContestants;
    $this->matches_per_round = ceil(($this->numContestants) / 2);
    $this->slots = $this->matches_per_round * 2;
    $this->matches_total = (pow($this->slots, 2) - $this->slots) / 2;
    $this->rounds_total = $this->matches_total / $this->matches_per_round;
    $this->tournament = $tournament;
  }

  /**
   * Options for this plugin.
   */
  public function optionsForm(&$form_state) {
    $this->getPluginOptions();
    $options = $this->pluginOptions;
    $plugin_options = array_key_exists(get_class($this), $options) ? $options[get_class($this)] : array();

    $form['max_team_play'] = array(
      '#type' => 'textfield',
      '#size' => 10,
      '#title' => t('Maximum times teams may play each other'),
      '#description' => t('Number of times team A will be allowed to play team B'),
      '#default_value' => array_key_exists('max_team_play', $plugin_options) ? $plugin_options['max_team_play'] : 1,
    );

    return $form;
  }

  /**
   * Theme implementations specific to this plugin.
   *
   * @see hook_theme()
   * @see tourney_theme()
   * @see _tourney_theme()
   */
  public static function theme($existing, $type, $theme, $path) {
    return array(
      'tourney_regularseason_standings' => array(
        'variables' => array('plugin' => NULL),
        'path' => $path . '/theme',
        'file' => 'regularseason.inc',
        'template' => 'tourney-regularseason-standings',
      ),
      'tourney_regularseason' => array(
        'variables' => array('plugin' => NULL),
        'path' => $path . '/theme',
        'file' => 'regularseason.inc',
        'template' => 'tourney-regularseason',
      ),
    );
  }

  /**
   * Preprocess variables for the template passed in.
   *
   * Tourney module will inform our plugin here which template (a tournament,
   * match, etc) is currently being processed. We inject our own
   * customizations via running our own plugin specific themes.
   *
   * @param $template
   *   The name of the template that is being preprocessed.
   * @param $vars
   *   The vars array to add variables to.
   *
   * @see template_preprocess_tourney_tournament_render()
   * @see template_preprocess_tourney_match_render()
   * @see template_preprocess_tourney_contestant()
   */
  public function preprocess($template, &$vars) {
    if ($template == 'tourney-tournament-render') {
      $vars['classes_array'][] = 'tourney-tournament-regularseason';
      $vars['header'] = theme('tourney_regularseason_standings', array('plugin' => $this));
      $vars['matches'] = theme('tourney_regularseason', array('plugin' => $this));
    }
    if ($template == 'tourney-match-render') {
      $last_match = count($vars['plugin']->structure['round-1']['matches']);
      if ($vars['match']['roundMatch'] == 1) {
        $vars['classes_array'][] = 'first';
      }
      if ($vars['match']['roundMatch'] == $last_match) {
        $vars['classes_array'][] = 'last';
      }
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

    // Calculate the maximum number of rounds.
    $this->getPluginOptions();
    $options = $this->pluginOptions;
    $plugin_options = array_key_exists(get_class($this), $options) ? $options[get_class($this)] : array();
    if (!empty($plugin_options) && $plugin_options['max_team_play']) {
      $this->rounds_multiplier = $plugin_options['max_team_play'];
    }

    $this->buildBrackets();
    $this->buildMatches();
    $this->buildGames();

    $this->data['contestants'] = array();

    // Calculate and set the match pathing
    $this->populatePositions();
    // Set in the seed positions
    $this->populateSeedPositions();
  }

  public function buildBrackets() {
    $this->data['brackets']['main'] = $this->buildBracket(array(
      'id' => 'main',
      'rounds' => $this->rounds_total * $this->rounds_multiplier,
    ));
  }

  /**
   * Populate rounds and matches into our data property.
   *
   * @see TourneyController::buildRound().
   * @see TourneyController::buildMatch().
   */
  public function buildMatches() {
    $match = &drupal_static('match', 0);
    $round = &drupal_static('round', 0);

    $max_rounds = $this->rounds_total * $this->rounds_multiplier;

    // Iterate through each round, creating the round data.
    for ($round = 1; $round <= $max_rounds; $round++) {
      // Add current round information to the data array
      $this->data['rounds'][$round] = $this->buildRound(array(
        'id' => $round,
        'bracket' => 'main',
        'matches' => $this->matches_per_round,
      ));
      // Add in all matches and their information for this round
      foreach (range(1, $this->matches_per_round) as $roundMatch) {
        $this->data['matches'][++$match] = $this->buildMatch(array(
          'id' => $match,
          'round' => $round,
          'tourneyRound' => $round,
          'roundMatch' => (int) $roundMatch,
          'bracket' => 'main',
        ));
      }
    }

  }

  /**
   * @see TourneyController::buildGame().
   */
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

  /**
   * Find and populate next/previous match pathing on the matches data array for
   * each match.
   */
  public function populatePositions() {
    foreach ($this->data['matches'] as $id => &$match) {
      $nextMatch = &$this->find('matches', array(
        'round'      => $match['round'] + 1,
        'roundMatch' => (int) ceil($match['roundMatch'] / 2)
      ), TRUE);
      if (!$nextMatch) continue;
      $slot = $match['id'] % 2 ? 1 : 2;
      $match['nextMatch']['winner'] = array(
        'id' => $nextMatch['id'], 
        'slot' => $slot
      );
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
  public function structure() {
    $structure = array();
    // Loop through our rounds and set up each one
    foreach ($this->data['rounds'] as $round) {
      $structure[$round['id']] = $round + array('matches' => array());
    }
    // Loop through our matches and add each one to its related round
    if (!is_array($this->data['matches'])) {
      throw new Exception('No matches found.');
    }
    // @todo why is this complaining about scalar? http://tourney/tourney/tournament/5
    foreach ($this->data['matches'] as $match) {
      $structure['round-' . $match['round']]['matches'][$match['id']] = $match;
    }
    $this->structure = $structure;

    return $this->structure;
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
