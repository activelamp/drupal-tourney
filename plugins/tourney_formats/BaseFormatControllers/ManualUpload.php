<?php

/**
 * @file
 * A Manual Upload class for tournaments. Constructs a single round with
 * match construction specified by a csv file which is uploaded during
 * tournament creation.
 */

/**
 * A class defining how matches are created for this style tournament.
 */

class ManualUploadController extends TourneyController {
  // Rules will examine this when deciding to fire off matchIsWon logic.
  public $moveWinners = FALSE;
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
  // Uploaded schema.
  public $uploadSchema;

  /**
   * Constructor
   */
  public function __construct($numContestants, $tournament = NULL) {
    parent::__construct();
    $this->tournament = $tournament;
    // Set our contestants, and then calculate the slots necessary to fit them
    if ($numContestants) {
      $this->numContestants = $numContestants;
      $this->matchesPerRound = ceil(($this->numContestants) / 2);
      $this->slots = (int) $this->matchesPerRound * 2;
      $this->matchesTotal = (pow($this->slots, 2) - $this->slots) / 2;
      //$this->roundsTotal = $this->matchesTotal / $this->matchesPerRound;
      $this->roundsTotal = 1;
    }

    // Flag used in rules to not fire off matchIsWon logic.
    $this->moveWinners = FALSE;
  }

  /**
   * Options for this plugin.
   *
   * @see tourney_get_plugin_options()
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
      '#disabled' => !empty($form_state['tourney']->id) ? TRUE : FALSE,
      '#element_validate' => array('manualupload_match_times_validate'),
    );
    $form['match_lineup_file'] = array(
      //'#name' => 'bob',
      '#type' => 'managed_file',
      '#title' => t('Choose a file'),
      '#size' => 22,
      '#element_validate' => array('manualupload_match_lineup_validate'),
      //'#required' => TRUE,
    );

    return $form;
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
      $vars['classes_array'][] = 'tourney-tournament-manualupload';

      $vars['header'] = theme('tourney_manualupload_standings', array('plugin' => $this));
      $vars['matches'] = theme('tourney_manualupload', array('plugin' => $this));
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
      'tourney_manualupload_standings' => array(
        'variables' => array('plugin' => NULL),
        'path' => $path . '/theme',
        'file' => 'manualupload.inc',
        'template' => 'tourney-manualupload-standings',
      ),
      'tourney_manualupload' => array(
        'variables' => array('plugin' => NULL),
        'path' => $path . '/theme',
        'file' => 'manualupload.inc',
        'template' => 'tourney-manualupload',
      ),
    );
  }

  /**
   * This builds the data array for the plugin. The most important data structure
   * your plugin should implement in build() is the matches array. It is from
   * this array that matches are saved to the Drupal entity system using
   * TourneyController::saveMatches().
   *
   * @see TourneyTournamentEntity::save()
   * @see TourneyController::saveMatches()
   */
  public function build() {
    // Reset the static vars.
    parent::build();
    drupal_static_reset('rr_matches');

    // Calculate the maximum number of rounds.
    $this->getPluginOptions();
    $options = $this->pluginOptions;
    $plugin_options = array_key_exists(get_class($this), $options) ? $options[get_class($this)] : array();
    if (!empty($plugin_options) && $plugin_options['max_team_play']) {
      $this->roundsMultiplier = (int)$plugin_options['max_team_play'];
    }
    // Parse the uploaded file. Drupal should delete this temporary file
    // after about 6 hours so we don't need to worry about it.
    $this->uploadSchema = $this->parseUploadFile($plugin_options['match_lineup_file']['fid']);

    $this->buildBrackets();
    $this->buildMatches();
    $this->buildGames();

    $this->data['contestants'] = array();

    // Calculate and set the match pathing
    $this->populatePositions();
    // Set in the seed positions
    $this->populateSeedPositions();
  }

  /**
   * @see manualupload_parse_file()
   */
  public function parseUploadFile($fid) {
    $report = manualupload_parse_file($fid);

    return $report;
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
    $slots = $this->slots;
    $match = &drupal_static('match', 0);
    $max_rounds = $this->roundsTotal * $this->roundsMultiplier;
    $matches_per_round = count($this->uploadSchema['rows']);

    // Calculate and iterate through rounds and their matches based on slots
    for ($round = 1; $round <= $max_rounds; $round++) {
      // Add current round information to the data array
      $this->data['rounds'][$round] = $this->buildRound(array(
        'id' => $round,
        'bracket' => 'main'
      ));
      // Add in all matches and their information for this round
      foreach (range(1, $matches_per_round) as $roundMatch) {
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
      $contestants = array_flip($this->uploadSchema['contestants']);
      $team_fields = $this->uploadSchema['team_fields'];
      foreach($this->uploadSchema['rows'] as $key => $row) {
        $matches[$key][1] = $contestants[$row[$team_fields[0]]];
        $matches[$key][2] = $contestants[$row[$team_fields[1]]];
      }
      // Extend matches by the round multiplier.
      $matches_new = array(NULL);
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
    drupal_add_js($this->pluginInfo['path'] . '/theme/manualupload_match_times_validate.js');
    return theme('tourney_tournament_render', array('plugin' => $this));
  }

}


/**
 * Element validate callback.
 *
 * @see ManualUploadController::optionsForm()
 */
function manualupload_match_times_validate($element, &$form_state) {
  if (empty($element['#value']) && $form_state['values']['format'] == 'ManualUploadController'
    || !empty($element['#value']) && is_numeric(parse_size($element['#value']))
    && $element['#value'] < 1 && $form_state['values']['format'] == 'ManualUploadController') {
    form_error($element, t('Maximum times teams may play each other must be a numeric value 1 or more.'));
  } 
}

function manualupload_match_lineup_validate($element, &$form_state) {
  $fid = $form_state['values']['plugin_options']['ManualUploadController']['match_lineup_file']['fid'];
  $schema = manualupload_parse_file($fid);
  $form_state['values']['players'] = count($schema['contestants']);
}

/**
 * Parse the uploaded file into a single nested array of usable information.
 *
 * @param int $fid
 *   The drupal file id of an uploaded file stored in the temporary path.
 *
 * @return
 *   An associative array:
 *   - rows: (array) The original CSV file's rows. Each element in this
 *     array is further broken down into a sub array of the fields in each
 *     row.
 *   - meta: (array) The first row in the original CSV file. Each element
 *     in the array will contain a column name.
 *   - contestants: (array) 1 based, of contestant identifier strings.
 *   - team_fields: (array):
 *     - 0: (int) field of row that contains first reference to contestant.
 *     - 1: (int) field of row that contains second reference to contestant.
 */
function manualupload_parse_file($fid) {
  $report = array();
  $file = file_load($fid);
  $file_contents = file_get_contents($file->uri);
  $file_lines = explode("\n", $file_contents);
  // Determine which columns contain the team players.
  $file_info = str_getcsv($file_lines[0]);
  $report['meta'] = $file_info;
  $teams_meta = array();
  foreach($file_info as $key => $value) {
    if (strtoupper($value) == 'TEAM') {
      $teams_meta[] = $key;
    }
  }
  $report['team_fields'] = $teams_meta;
  // Generate a 1 based array of all contestants. The numeric keys will
  // be used later on as the contestant placeholder for seeding.
  $file_contestants = array();
  for($x=1; $x<sizeof($file_lines); $x++) {
    $elements = str_getcsv($file_lines[$x]);
    $report['rows'][] = $elements;
    $file_contestants[$elements[$teams_meta[0]]] = TRUE;
    $file_contestants[$elements[$teams_meta[1]]] = TRUE;
  }
  $file_contestants = array_keys($file_contestants);
  array_unshift($file_contestants, FALSE);
  unset($file_contestants[0]);
  $report['contestants'] = $file_contestants;

  return $report;
}

