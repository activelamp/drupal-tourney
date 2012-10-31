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
    $this->calculateSeeds();
    foreach ($this->data['matches'] as $id => &$match) {
      $slot1 = $this->calculateNextPosition($match, 1);
      $slot2 = $this->calculateNextPosition($match, 2);
      if ($slot1) {
        $match['nextMatch'][1] = $slot1;
        $this->data['matches'][$slot1['id']]['previousMatches'][$slot1['slot']] = $id;
      }
      if ($slot2) {
        $match['nextMatch'][2] = $slot2;
        $this->data['matches'][$slot2['id']]['previousMatches'][$slot2['slot']] = $id;
      }
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
    foreach ($this->data['seeds'] as $mid => $seeds) {
      if ($this->data['matches'][$mid]['round'] == 1) {
        $match =& $this->data['matches'][$mid];
        $match['seeds'] = $seeds;
        $match['bye'] = $seeds[2] === NULL;
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
      $structure[$round['id']] = $round; // + array('matches' => array());
      $structure[$round['id']]['matches'] = array();
    }
    // Loop through our matches and add each one to its related round
    foreach ($this->data['matches'] as $match) {
      $structure['round-' . $match['round']]['matches'][$match['id']] = $match;
    }
    $this->structure = $structure;

    return $this->structure;
  }

  /**
   * Figures out what seed position is playing in every match of the tournament.
   *
   * Creates a keyed array with the key being match number and slots array as
   * the value, and the seed position as the values of that array.
   */
  public function calculateSeeds() {
    $matches = &drupal_static('rr_matches', array());
    $mid = 1;
    $slots = (int) $this->slots;

    if (empty($matches)) {
      $matches = array();
      foreach (range(1, $slots - 1) as $round) {
        $list = range(2, $slots);
        $list = array_merge(array(1), array_slice(array_merge($list, $list), $slots-$round, $slots-1));
        foreach (range(1, $slots / 2) as $match) {
          $match = array($list[$match-1], $list[$slots-$match]);

          // Slot positions need to flip every other round.
          $match = ($round % 2) ? $match : array_reverse($match);

          // Make the array 1-based
          array_unshift($match, NULL);
          unset($match[0]);

          $matches[$mid++] = $match;
        }
      }
      $matches_new[] = NULL;
      for ($x=0; $x<$this->rounds_multiplier; $x++) {
        foreach ($matches as $match) {
          $matches_new[] = $match;
        }
      }
      unset($matches_new[0]);
      $matches = $matches_new;
    }
    return $this->data['seeds'] = $matches;
  }

  /**
   * Given a match info array, returns both the target match and slot.
   *
   * @param $match_info
   *   The match data array.
   * @param $slot
   *   Slot placement, one-based.
   * @return $result
   *   Keyed array giving both the target match and slot
   */
  function calculateNextPosition($match_info, $slot) {
    $seeds = $this->data['seeds'];
    $place = $match_info['id'];

    // Get the current contestant slot number
    $id = $seeds[$place][$slot];
    foreach ($seeds as $mid => $slots) {
      if ($mid <= $place) {
        continue;
      }
      // Check for the next instance of it after the current match
      if (in_array($id, $slots)) {
        $slots = array_flip($slots);
        return array('id' => $mid, 'slot' => $slots[$id]);
      }
    }
    return NULL;
  }

  public function render($style = 'tree') {
    // Build our data structure
    $this->build();
    $this->structure();
    drupal_add_js($this->pluginInfo['path'] . '/theme/regularseason.js');
    return theme('tourney_tournament_render', array('plugin' => $this));
  }

}
