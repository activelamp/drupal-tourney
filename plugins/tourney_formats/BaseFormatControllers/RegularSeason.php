<?php

/**
 * @file
 * Regular season controller, new system.
 */

/**
 * A class defining how matches are created, and rendered for this style
 * tournament.
 */
class RegularSeasonController extends RoundRobinController {
  // Number of contestants required per round to fill all matches for the
  // round.
  public $slots;
  // Total number of matches if each team only plays each other once.
  public $matchesTotal;
  // Total number of rounds if each team only plays each other once.
  public $roundsTotal;
  // Number of matches per round.
  public $matchesPerRound;
  // Number of times team A must play team B.
  public $roundsMultiplier = 1;

  /**
   * Constructor
   */
  public function __construct($numContestants, $tournament = NULL) {
    parent::__construct($numContestants, $tournament);
    // Set our contestants, and then calculate the slots necessary to fit them
    if ($numContestants) {
      $this->matchesPerRound = ceil(($this->numContestants) / 2);
      $this->slots = (int) $this->matchesPerRound * 2;
      $this->matchesTotal = (pow($this->slots, 2) - $this->slots) / 2;
      $this->roundsTotal = $this->matchesTotal / $this->matchesPerRound;
    }

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
    // Calculate the maximum number of rounds.
    $this->getPluginOptions();
    $options = $this->pluginOptions;
    $plugin_options = array_key_exists(get_class($this), $options) ? $options[get_class($this)] : array();
    if (!empty($plugin_options) && $plugin_options['max_team_play']) {
      $this->roundsMultiplier = $plugin_options['max_team_play'];
    }

    parent::build();
  }

  public function buildBrackets() {
    $this->data['brackets']['main'] = $this->buildBracket(array(
      'id' => 'main',
      'rounds' => $this->roundsTotal * $this->roundsMultiplier,
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

    $max_rounds = $this->roundsTotal * $this->roundsMultiplier;

    // Iterate through each round, creating the round data.
    for ($round = 1; $round <= $max_rounds; $round++) {
      // Add current round information to the data array
      $this->data['rounds'][$round] = $this->buildRound(array(
        'id' => $round,
        'bracket' => 'main',
      ));
      // Add in all matches and their information for this round
      foreach (range(1, $this->matchesPerRound) as $roundMatch) {
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
   * Figures out what seed position is playing in every match of the tournament.
   *
   * Creates a keyed array with the key being match number and slots array as
   * the value, and the seed position as the values of that array.
   */
  public function calculateSeeds() {
    $matches = parent::calculateSeeds();
    if (!empty($matches)) {
      $matches_new[] = NULL;
      for ($x=0; $x<$this->roundsMultiplier; $x++) {
        foreach ($matches as $match) {
          $matches_new[] = $match;
        }
      }
      unset($matches_new[0]);
      $matches = $matches_new;
    }
    return $this->data['seeds'] = $matches;
  }

  public function render() {
    // Build our data structure
    $this->build();
    $this->structure();
    drupal_add_js($this->pluginInfo['path'] . '/theme/regularseason.js');
    return theme('tourney_tournament_render', array('plugin' => $this));
  }
}
