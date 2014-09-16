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

    form_load_include($form_state, 'php', 'tourney', 'plugins/tourney_formats/BaseFormatControllers/ManualUpload');

    $form['match_lineup_file'] = array(
      '#type' => 'managed_file',
      '#title' => t('Choose a file'),
      '#description' => t('A csv file to specify which teams play on matches. Each row is a match. First row must contain column name description. Column name description for team\'s columns must contain the word \'team\'.'),
      '#default_value' => array_key_exists('match_lineup_file', $plugin_options) ? $plugin_options['match_lineup_file'] : 0,
      '#disabled' => !empty($form_state['tourney']->id) ? TRUE : FALSE,
      '#size' => 22,
      "#upload_validators" => [
        'file_validate_extensions' => ['csv txt'], 
        'manualupload_upload_validate' => [],
      ],
      '#element_validate' => ['manualupload_file_validate'],
    );
    $form['max_team_play'] = array(
      '#type' => 'textfield',
      '#size' => 10,
      '#title' => t('Rounds multiplier.'),
      '#description' => t('Uploaded team match schedule will be multiplied by this, with each schedule entering its own round.'),
      '#default_value' => array_key_exists('max_team_play', $plugin_options) ? $plugin_options['max_team_play'] : 1,
      '#disabled' => !empty($form_state['tourney']->id) ? TRUE : FALSE,
      '#element_validate' => array('manualupload_match_times_validate'),
    );
    if (isset($plugin_options['file_schema'])) {
      $form['file_schema'] = array(
        '#type' => 'hidden',
        '#default_value' => $plugin_options['file_schema'],
        '#disabled' => TRUE,
      );
    }

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

      // @todo: These calls are pretty expensive. Add settings to turn this on.
      // $vars['header'] = theme('tourney_manualupload_standings', array('plugin' => $this));
      // $vars['matches'] = theme('tourney_manualupload', array('plugin' => $this));
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
    // If this class is extended the only way to get our (manualupload) theme
    // to operate is to set the path by hand, otherwise the external module
    // that has extended us will try to open the following files it its own
    // directory.
    $path = drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/manual_upload';
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
    // Write the file schema into the plugin options. Now the file needs
    // no longer to exist. @see tourney_initialize_configuration(), ultimately
    // these options get pulled and written from the 'tourney' table.
    if (!empty($plugin_options) && !isset($plugin_options['file_schema'])) {
      try {
        $plugin_options['file_schema'] = $this->parseUploadFile($plugin_options['match_lineup_file']['fid']);
        $this->tournament->set(get_class($this), $plugin_options);
      } catch (Exception $e) {
        drupal_set_message("Missing schema file for tournament {$this->tournament->id}: " . $e->getMessage(), 'warning');
      }
    }
    $this->uploadSchema = $plugin_options['file_schema'];

    $this->buildBrackets();
    $this->buildMatches();
    $this->buildGames();

    $this->data['contestants'] = array();
  }

  /**
   * @see manualupload_parse_csv()
   */
  public function parseUploadFile($fid) {
    if (!is_numeric($fid)) {
      throw new Exception('Invalid parameter. File identification must be integer.');
    }
    if (!$file = file_load(intval($fid))) {
      throw new Exception('File can not be loaded from FID.');
    };
    if (!$file_contents = file_get_contents($file->uri)) {
      throw new Exception('File can not be read or is empty.');
    };

    return manualupload_parse_csv($file_contents);
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
    // drupal_add_js($this->pluginInfo['path'] . '/theme/manualupload_match_times_validate.js');
    return theme('tourney_tournament_render', array('plugin' => $this));
  }

  /**
   * Return the number of players stored in the plugin options.
   *
   * @return
   *   (int) Number of players in this tournament as configured by plugin.
   */
  public function getNumberOfPlayers() {
    // Plugin options should be keyed by class name. If ManualUploadController
    // has been extended then use the called class's name as the key.
    $class = get_called_class();
    return $this->pluginOptions[$class]['players'];
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
    form_error($element, t('Maximum rounds must be a numeric value 1 or more.'));
  }
}

/**
 * Element validate callback.
 *
 * @see ManualUploadController::optionsForm()
 */
function manualupload_upload_validate(stdClass $file) {
  $errors = [];

  $file_contents = file_get_contents($file->uri);
  try {
    $report = manualupload_parse_csv($file_contents);    
  }
  catch (Exception $e) {
    $report = FALSE;
    $errors[] = $e->getMessage();
  }

  return $errors;
}

/**
 * Callback for #element_validate.
 *
 * @see ManualUploadController::optionsForm()
 */
function manualupload_file_validate($element, &$form_state) {
  $tournament = array_key_exists('tourney', $form_state) ? $form_state['tourney'] : NULL;

  $plugin_name = $form_state['values']['format'];
  $plugin_options = &$form_state['values']['plugin_options'][$plugin_name];
  $file_schema = NULL;

  // Try to get the tournament match schema from tournament options.
  if ($tournament && $plugin_name) {
    $plugin = $tournament->tourneyFormatPlugin;
    if ($plugin !== NULL) {
      $options = $plugin->getPluginOptions();
      $tournament_plugin_options = array_key_exists($plugin_name, $options) ? $options[$plugin_name] : [];
      if (!empty($plugin_options) && array_key_exists('file_schema', $tournament_plugin_options)) {
        $file_schema = $tournament_plugin_options['file_schema'];
      }
    }
  }

  // Try to get the tournament match schema from an uploaded file.
  if ($file_schema === NULL && array_key_exists('match_lineup_file', $plugin_options)) {
    $file = file_load(intval($plugin_options['match_lineup_file']['fid']));
    if ($file) {
      $file_contents = file_get_contents($file->uri);
      $file_schema = manualupload_parse_csv($file_contents);        

      $contestants = $file_schema['contestants'];
      if (count($contestants) > 0) {
        $plugin_options['players'] = count($contestants);
      }
      else {
        form_error($element, t('Number of players must be greater than zero.'));
      }
    }
  }
}

/**
 * Parse a CSV file's contents into a single nested array of usable information.
 *
 * @param string $contents
 *   The contents of the CSV file to be parsed
 *
 * @return
 *   An associative array:
 *     - meta: (array) The first row in the original CSV file. Each element in
 *             the array will contain a column name.
 *     - fields: (array) An array of matches of column names to field names.
 *       - field_name: (array) An array of column name keys where the field
 *                     refers to a column name.
 *     - rows: (array) The original CSV file's rows. Each element in this array
 *             is further broken down into a sub-array of fields in each row.
 *     - contestants: (array) 1 based array of contestant identifier strings.
 */
function manualupload_parse_csv($contents) {
  $report = [];
  $lines = explode("\n", $contents);

  if (empty($lines)) {
    throw new Exception('File is empty or of the wrong format.');
  }
  
  $report['meta'] = array_map('strtolower', str_getcsv(array_shift($lines)));
  if (empty($report['meta'])) {
    throw new Exception('Could not locate file metadata (column names).');
  }

  $report['fields'] = [];
  $required_fields = ['team', 'time', 'date'];

  foreach ($required_fields as $field) {
    $fields = array_keys($report['meta'], $field);
    if (empty($fields)) {
      throw new Exception("Could not locate $field column(s).");
    }
    $report['fields'][$field] = $fields;
  }

  $report['rows'] = [];
  $contestants = [];
  foreach ($lines as $line) {
    $row = str_getcsv($line);

    $timestring = strtotime($row[reset($report['fields']['time'])] + ' ' + $row[reset($report['fields']['date'])]);
    if ($timestring === FALSE) {
      throw new Exception("Unix timestamp can not be constructed from supplied text string: $timestring");
    }

    foreach ($report['fields']['team'] as $team_field) {
      $contestants[$row[$team_field]] = TRUE;
    }
    $report['rows'][] = $row;
  }
  if (empty($contestants)) {
    throw new Except('Could not locate team contestants.');
  }
  $contestants = array_keys($contestants);
  array_unshift($contestants, NULL);
  unset($contestants[0]);
  $report['contestants'] = $contestants;

  return $report;
}
