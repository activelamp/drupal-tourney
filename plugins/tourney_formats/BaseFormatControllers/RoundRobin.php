<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */

/**
 * A class defining how matches are created for this style tournament.
 */

class RoundRobinController extends TourneyController {
  public $moveWinners = FALSE;

  /**
   * Constructor
   */
  public function __construct($numContestants, $tournament = NULL) {
    parent::__construct();
    // Set our contestants, and then calculate the slots necessary to fit them
    $this->numContestants = $numContestants;
    $this->slots = $numContestants % 2 ? $numContestants + 1 : $numContestants;
    $this->tournament = $tournament;

    // Flag used in rules to not fire off matchIsWon logic.
    $this->moveWinners = FALSE;
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
      $vars['classes_array'][] = 'tourney-tournament-roundrobin';

      $vars['header'] = theme('tourney_roundrobin_standings', array('plugin' => $this));
      $vars['matches'] = theme('tourney_roundrobin', array('plugin' => $this));
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
   * Theme implementations to register with tourney module.
   *
   * @see hook_theme().
   */
  public static function theme($existing, $type, $theme, $path) {
    return array(
      'tourney_roundrobin_standings' => array(
        'variables' => array('plugin' => NULL),
        'path' => $path . '/theme',
        'file' => 'roundrobin.inc',
        'template' => 'tourney-roundrobin-standings',
      ),
      'tourney_roundrobin' => array(
        'variables' => array('plugin' => NULL),
        'path' => $path . '/theme',
        'file' => 'roundrobin.inc',
        'template' => 'tourney-roundrobin',
      ),
    );
  }

  /**
   * This builds the data array for the plugin. The most important data structure
   * your plugin should implement in build() is the matches array. It is from
   * this array that matches are saved to the Drupal entity system using
   * TourneyController::saveMatches().
   */
  public function build() {
    // Reset the static vars.
    parent::build();
    drupal_static_reset('rr_matches');
    
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
      'rounds' => $this->slots - 1,
    ));
  }

  public function buildMatches() {
    $slots = $this->slots;
    $match = &drupal_static('match', 0);

    // Calculate and iterate through rounds and their matches based on slots
    for ($round = 1; $round < $slots; $round++) {
      // Add current round information to the data array
      $this->data['rounds'][$round] =
        $this->buildRound(array('id' => $round, 'bracket' => 'main'));

      // Add in all matches and their information for this round
      foreach (range(1, $slots / 2) as $roundMatch) {
        $this->data['matches'][++$match] =
          $this->buildMatch(array(
            'id' => (int) $match,
            'round' => (int) $round,
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
   * Figures out what seed position is playing in every match of the tournament.
   *
   * Creates a keyed array with the key being match number and slots array as
   * the value, and the seed position as the values of that array.
   */
  public function calculateSeeds() {
    $matches = &drupal_static('rr_matches', array());
    $mid = 1;
    $slots = $this->slots;

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
    }
    return $this->data['seeds'] = $matches;
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
      $structure[$round['id']] = $round + array('matches' => array());
    }
    // Loop through our matches and add each one to its related round
    foreach ($this->data['matches'] as $match) {
      $structure['round-' . $match['round']]['matches'][$match['id']] = $match;
    }
    $this->structure = $structure;

    return $this->structure;
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

  public function render() {
    // Build our data structure
    $this->build();
    $this->structure();
    drupal_add_js($this->pluginInfo['path'] . '/theme/roundrobin.js');
    return theme('tourney_tournament_render', array('plugin' => $this));
  }
}
