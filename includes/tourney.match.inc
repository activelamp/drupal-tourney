<?php

/**
 * @file
 * Defines the wrapper functions for Entity API CRUD specifically for the match
 * entity. To used these function make sure you module_load_include().
 *
 */

/**
 * Show the tournament page.
 */
function tourney_match_page_view($match) {
  return $match->view('full', NULL, TRUE);
}

/**
 * Page callback. Edit a match.
 */
function tourney_match_edit($match) {
  drupal_set_title(t('Edit Match: @title', array('@title' => $match->label)));

  return drupal_get_form('tourney_match_form', $match);
}

function tourney_game_win_form_submit($form, &$form_state) {
  $extra = $form_state['values']['extra'];
  $contestant = $form_state['input']['contestant'];
  $match = tourney_match_load($extra['match']);
  $game = tourney_game_load($extra['game']);
  $game->winner = $contestant;
  tourney_game_save($game);
  $form_sate['redirect'] = $match->getUri();
}

function tourney_game_win_form($form, &$form_state, $extra) {
  $form = array(
    'extra' => array(
      '#type' => 'value',
      '#value' => $extra,
    ),
    'contestant' => array(
      '#type' => 'hidden',
      '#value' => $extra['contestant'],
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'Win',
      '#attributes' => array('class' => array('win-button')),
    ),
  );
  return $form;
}

/**
 * The match form.
 */
function tourney_match_form($form, &$form_state, $match) {
  $match->type = 'match';
  $form_state['match'] = $match;

  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('The title you enter here appears on the page.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => $match->label,
    '#weight' => -10,
  );

  module_load_include('inc', 'tourney', 'includes/tourney.tournament');
  $games = tourney_tournament_get_match_games($match);

  $canset = $match->getTournament()->canSetContestants($match->getMID());
  $contestants = $match->getMatchContestants();

  $cid = 0;
  foreach (array(1,2) as $slot) {
    $contestant = $match->getContestant($slot);
    $eid = is_object($contestant) ? $contestant->eid : null;
    $form['contestant' . $slot] = array(
      '#title' => check_plain(t('Contestant ' . $slot)),
      '#default_value' => $eid,
      '#disabled' => !($slot & $canset),
    ) + _tourney_match_contestant($match, $eid);
  }

  $form['games'] = array(
    '#type' => 'textfield',
    '#title' => t('Games'),
    '#description' => t('Enter the number of games in this match'),
    '#size' => 4,
    '#maxlength' => 255,
    '#default_value' => $match->games,
    '#required' => TRUE,
    '#disabled' => $match->isFinished(),
  );

  if (array_key_exists('bye', $match->matchInfo) && $match->matchInfo['bye']) {
    $form['contestant1']['#title'] = t('Contestant');
    $form['contestant1']['#description'] = t('This contestant will receive a bye and automatically advance to the next round.');

    $form['contestant2'] = array(
      '#type' => 'value',
      '#value' => 0,
    );

    $form['games'] = array(
      '#prefix' => '<h2>' . t('Games') . '</h2>',
      '#suffix' => t('This match is a bye. No games to configure.'),
      '#type' => 'value',
      '#value' => 0,
    );
  }

  if (!array_key_exists('bye', $match->matchInfo) && $match->isLocked()) {
    $form['fix_games'] = array(
      '#markup' => "Sorry, one or more of the next matches have already started, you cannot edit this match's games anymore.",
    );
  }
  else {
    $form['fix_games'] = array(
      '#theme' => 'tourney_match_fix',
      '#games' => $match->games,
    );

    foreach ($match->getGames() as $id => $game) {
      if (!$game->getWinner()) {
        continue;
      }
      $form['fix_games']['game.' . $id] = array(
        '#type' => 'radios',
        '#title' => t('Game @id', array('@id' => $id)),
        '#default_value' => $game->winner,
        '#options' => tourney_format_contestant_options($game->getContestants()),
      );
    }
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
  );

  $form['additional_settings'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => 99,
  );

  $form['path'] = tourney_create_path_field($match);

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Publishing Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#weight' => 95,
  );

  $form['options']['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Published'),
    '#default_value' => isset($match->status) ? $match->status : 0,
  );

  field_attach_form('tourney_match', $match, $form, $form_state);

  $form['#submit'][] = 'tourney_match_form_submit';
  return $form;
}

/**
 * Create an entity field select box for contestants on the match edit form.
 *
 * @param $match
 *   The match object for the match being edited
 * @param $this_eid
 *   Optional eid of a contestant to prevent it from being sanitized
 * @return array
 *   Returns a form element to be used in a form.
 *
 * @see tourney_match_form().
 * @see _tourney_match_contestants().
 */
function _tourney_match_contestant($match, $this_eid = null) {
  return array(
    '#type' => 'select',
    '#options' => _tourney_match_contestants($match, $this_eid),
  );
}
/**
 * Get the bundles that can be contestants in a match and build an options
 * array to be used on the match edit form.
 *
 * @param $match
 *   The match object for the match being edited.
 * @param $this_eid
 *   Optional eid of a contestant to prevent it from being sanitized
 * @return array
 *   Returns an $options array of contestants that have not been added to a
 *   tournament yet.
 *
 * @see _tourney_match_contestant().
 * @see _tourney_match_contestants_sanitize().
 */
function _tourney_match_contestants($match, $this_eid = null) {
  $query = db_select('relation_bundles', 'rb');
  $query->fields('rb', array('entity_type', 'bundle'))
    ->distinct()
    ->condition('relation_type', array('contestant'))
    ->condition('r_index', 1);
  $results = $query->execute()->fetchAll();

  $options = array('' => ' - Choose contestant -');
  foreach ($results as $row) {
    $entity_type = $row->entity_type;
    $group = array();

    if ($row->bundle === '*') {
      $entities = entity_load($entity_type);
    }
    else {
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', $entity_type, '=');
      // Users don't have bundles, don't look for a bundle on user entities.
      if ($entity_type != 'user') {
        $query->propertyCondition('type', $row->bundle, '=');
      }
      $result = $query->execute();

      $entities = array();
      if (isset($result[$entity_type])) {
        $ids = array_keys($result[$entity_type]);
        $entities = entity_load($entity_type, $ids);
      }
    }

    if (empty($entities)) {
      $group[] = ' - Empty - ';
    }
    else {
      foreach ($entities as $eid => $entity) {
        $title = $entity->entity_name;
        $group["$entity_type.$eid"] = $title;
      }
    }
    if (!empty($group)) {
      $group_name = $row->bundle !== '*' ? $entity_type . ' - ' . $row->bundle : $entity_type;
      $options[$group_name] = $group;
    }
  }
  // Do not sanitize the list for "No Automation"
  if (!$match->getTournament()->get('no_automation', FALSE)) {
    _tourney_match_contestants_sanitize($match, $this_eid, $options);
  }
  return $options;
}

/**
 * Helper function for a select element options array.
 *
 * Ensure that the contestant is not related to any other match in the first
 * round. Remove contestant if already associated with a match in first round.
 * This means that they are already playing in the tournament.
 *
 * @param $match
 *   The match object the select options will display on.
 * @param $this_eid
 *   Optional eid of a contestant to prevent it from being sanitized
 * @param $options
 *   The options array for the select form element.
 *
 * @see _tourney_match_contestant().
 */
function _tourney_match_contestants_sanitize($match, $this_eid, &$options) {
  $tournament = $match->getTournament();
  $plugin = $tournament->tourneyFormatPlugin;

  // Get all the matches in round 1 for this tournament
  $tourney_matches = $plugin->data['matches'];
  $match_ids = array();
  foreach ($tourney_matches as $m) {
    if ($m['round'] == 1) {
      $tmatch = $tournament->getMatchById($m['id']);
      if (is_object($tmatch)) {
        $match_ids[] = $tmatch->id;
      }
    }
  }
  $matches = tourney_match_load_multiple($match_ids);

  // Unset users in the options array that are already selected
  $contestants = $match->getTournament()->getContestants();
  unset($contestants[$this_eid]);
  foreach ($match->getContestantIds() as $contestant) {
    // Don't want to remove contestants that are part of this match.
    if (isset($match->matchInfo['seeds']) && $match->matchInfo['seeds']) {
      if ( !isset($match->matchInfo['seeds'][$contestant['slot']]) ) continue;
    }
    unset($contestants[$contestant['eid']]);
  }
  foreach ($contestants as $eid => $contestant ) {
    foreach (array_keys($options) as $group) {
      if (is_array($options[$group]) && array_key_exists($eid, $options[$group])) {
        unset($options[$group][$eid]);
      }
    }
  }

  // special case naming occurs with different bundled node types
  $userkey = array_key_exists('user', $options) ? 'user' : (array_key_exists('user - user', $options) ? 'user - user' : NULL);
  // Remove admin accounts from the contestant choices
  if ($userkey) {
    foreach ($options[$userkey] as $id => $user) {
      if (str_replace("user.", "", $id) <= 1) {
        unset($options[$userkey][$id]);
      }
    }
  }
}

/**
 * Validation function for matches
 */
function tourney_match_form_validate($form, &$form_state) {
  if (!is_int($form_state['values']['games']) && $form_state['values']['games'] % 2 === 0) {
    form_set_error('games', t('The number of games must be an odd number'));
  }
  if (!empty($form_state['values']['contestant1']) && !empty($form_state['values']['contestant2']) && $form_state['values']['contestant1'] === $form_state['values']['contestant2']) {
    form_set_error('contestant1', t('Contestant 1 and Contestant 2 cannot be the same person.'));
  }
  $wins = array();
  foreach ($form_state['input'] as $id => $value) {
    if (!$value || substr($id, 0, 5) != 'game_') continue;
    if (!array_key_exists($value, $wins)) $wins[$value] = 0;
    $wins[$value]++;
  }
  foreach ($wins as $id => $count) {
    if ($count - 1 > $form_state['match']->games / 2)
      form_set_error('', t('A contestant cannot win more games than neccesary to win a match'));
  }
}

/**
 * Match form submit handler.
 */
function tourney_match_form_submit($form, &$form_state) {
  $match = $form_state['match'];
  $match_reset = FALSE;
  
  // Check to see if the contestants on this match are different than who is
  // currently associated with the match. If they are different, we need to
  // reset the match.
  $contestant_ids = array_keys($match->getMatchContestants());
  if (!empty($contestant_ids) 
    && (!in_array($form_state['values']['contestant1'], $contestant_ids)
      || !in_array($form_state['values']['contestant2'], $contestant_ids))) {
    
    $match->resetMatch();
    $match_reset = TRUE;
    drupal_set_message('The games in this match have been reset');
  }

  if (isset($form_state['input']['label']) && $form_state['input']['label'] !== '%AutoEntityLabel%') {
    $match->label = $form_state['input']['label'];
  }
  if (array_key_exists('contestant1', $form_state['input'])) {
    if ($eid = $form_state['input']['contestant1']) {
      @list($type, $id) = explode('.', $eid);
      $entity = entity_load_single($type, $id);
      $match->addContestant($entity, 1);
    }
    else {
      $contestant = $match->getContestant(1);
      $match->removeContestant($contestant);
    }
  }
  if (array_key_exists('contestant2', $form_state['input'])) {
    if ($eid = $form_state['input']['contestant2']) {
      @list($type, $id) = explode('.', $eid);
      $entity = entity_load_single($type, $id);
      $match->addContestant($entity, 2);
    }
    else {
      $contestant = $match->getContestant(2);
      $match->removeContestant($contestant);
    }
  }

  // This is here to set the ->winner property. This way it's loaded in so the rule can fix undos
  $match->getWinner();
  foreach ($form_state['input'] as $id => $value) {
    // If we're not dealing with a game move on.
    if (!$value || substr($id, 0, 5) != 'game_' || $match_reset) continue;

    $parts = explode('_', $id);
    $game = tourney_game_load($parts[1]);
    if ($game->winner != $value) {
      $game->winner = $value;
      tourney_game_save($game);
      // The winner needs to be re set for the match
      $match->winner = '';
    }
  }

  //support for autotitle involving contestant tokens
  if(module_exists('auto_entitylabel') && !empty($match->auto_entitylabel_applied) && $match->auto_entitylabel_applied == 1) {
    //the title was created prior to Contestants being added, we need to force the re-title.
    unset($match->auto_entitylabel_applied);
  }

  entity_form_submit_build_entity('tourney_match', $match, $form, $form_state);

  $match->save();

  $form_state['redirect'] = $match->getUri();
  drupal_set_message(t('Your match has been saved.'));
}

/**
 * Show the tournament page.
 */
function tourney_game_page_view($game) {
  return $game->view('full', NULL, TRUE);
}

/**
 * Page callback. Edit a match.
 */
function tourney_game_edit($game) {
  drupal_set_title(t('Edit Game: @title', array('@title' => $game->label)));

  return drupal_get_form('tourney_game_form', $game);
}

/**
 * Format objects to populate a select list
 *
 * @param $contestants
 *   An array of contestant objects to format.
 * @return array
 *   Returns an options array ready to be used in a #options form element.
 */
function tourney_format_contestant_options($contestants) {
  $options = array();
  foreach ($contestants as $id => $contestant) {
    $options[$id] = $contestant->entity_name;
  }
  return $options;
}

/**
 * The match form.
 */
function tourney_game_form($form, &$form_state, $game) {
  $game->type = 'game';
  $form_state['game'] = $game;
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('The title you enter here appears on the page.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => $game->label,
    '#weight' => -10,
  );

  $form['winner'] = array(
    '#type' => 'select',
    '#title' => t('Winner'),
    '#description' => t('The winner of this game.'),
    '#options' => array('' => t('- Choose Winner -')) + tourney_format_contestant_options($game->getContestants()),
    '#default_value' => isset($game->winner) ? $game->winner : -1,
  );

  $form['path'] = tourney_create_path_field($game);

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Publishing Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#weight' => 95,
  );

  $form['options']['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Published'),
    '#default_value' => isset($game->status) ? $game->status : 0,
  );

  $form['additional_settings'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => 99,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
  );

  field_attach_form('tourney_game', $game, $form, $form_state);

  $form['#submit'][] = 'tourney_game_form_submit';
  return $form;
}

/**
 * Game form submit handler.
 */
function tourney_game_form_submit($form, &$form_state) {
  drupal_set_message(t('Your game has been saved.'));

  $game = $form_state['game'];
  $game->label = $form_state['input']['label'];
  entity_form_submit_build_entity('tourney_game', $game, $form, $form_state);
  tourney_game_save($game);
  drupal_set_message(url($game->getUri()));

  $form_state['redirect'] = $game->getUri();
}

function tourney_match_get_parents($mid, $depth) {
  $ids = array($mid);

  $match = tourney_match_load($mid);
  $matches = $match->getTournament()->buildMatches();
  $bracket = tourney_get_name_part($match->this_match, 'bracket');

  $names = array($match->previous_match1, $match->previous_match2);
  while ($depth-- > 0) {
    $new_names = array();
    foreach ($names as $name) {
      $nmatch = tourney_get_match_by_name($matches, $name);
      if (!$nmatch) {
        continue;
      }
      if (tourney_get_name_part($nmatch->this_match, 'bracket') != $bracket) {
        continue;
      }
      $ids[] = $nmatch->id;
      $new_names = array_merge($new_names, array($nmatch->previous_match1, $nmatch->previous_match2));
    }
    $names = $new_names;
  }

  return $ids;
}

/**
 * @param $bracket_structure
 *   The bracket structure for this tournament.
 * @param $path
 *   The path to the next match in the bracket structure.
 * @return $array
 *   Returns the match object that is on the portion of the bracket stucture
 *   that contains the match object and a games array of game objects.
 */
function tourney_get_match_by_name($bracket_structure, $path) {
  if ($path == 'manual') {
    return NULL;
  }

  // Seperate the parts of the match path.
  $parts = explode('_', $path);

  // Get just the structure of the next match
  $next_match_structure = $bracket_structure[$parts[0]][$parts[1]];

  // Set $id to the round number
  @list(, $id) = explode('-', $parts[3]);

  /**
   * The bracket structure should be built in the order of matches. This for
   * loop assumes that the matches will always be in order of match-1, match-2,
   * and so on.
   *
   * Create an iterator for each match, and return the match that has the same
   * match number as the iterator.
   */
  $i = 1;
  foreach ($next_match_structure as $match) {
    if ($id == $i) {
      return $match['match'];
    }
    $i++;
  }
}

/**
 * Theme the match edit table of players
 */
function theme_tourney_match_fix($vars) {
  $element = $vars['element'];

  // Get the keys of the game form elements in this match render element
  // @see tourney_match_form().
  $children = element_children($element);

  // Initialize the header and rows
  $header = array('Team Name');
  $rows   = array();

  // Loop through all the games that have started.
  for ($gn = 0; $gn < $element['#games']; $gn++) {
    $header[] = "Game " . ($gn + 1);

    if ($gn < count($children)) {
      // Get the form element for this game number.
      $game = $element[$children[$gn]];
      foreach (element_children($game) as $user) {
        if (!isset($rows[$user])) {
          $rows[$user][] = $game[$user]['#title'];
        }
        // Render the form element as a column in this row of the table.
        $rows[$user][] = drupal_render($game[$user]);
      }
    }
    else {
      foreach ($rows as $id => &$row) {
        $row[] = ' - ';
      }
    }
  }

  $header[] = "Results";
  foreach ($rows as $id => &$row) {
    $row[] = ' - ';
  }

  if (!$rows) return null;

  return theme('table', array(
    'attributes' => array('id' => 'game-list'),
    'header' => $header,
    'rows' => $rows,
  ));
}
