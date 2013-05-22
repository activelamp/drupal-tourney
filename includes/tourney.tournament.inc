<?php

/**
 * @file
 * Adds tourament pages for page callbacks in hook_menu().
 */

/**
 * The tournament form.
 */
function tourney_tournament_form($form, &$form_state, $tourney) {
  // Set the bundle name
  $tourney->type = 'tournament';
  $form_state['tourney'] = $tourney;

  // Load the modal library and add the modal javascript.
  ctools_include('modal');
  ctools_modal_add_js();

  // Use the entity id as the configure form id, or generate random string.
  $random_id = !empty($form_state['input']) ? $form_state['input']['rounds_form_id'] : user_password(32);
  $form['rounds_form_id'] = $form['seeds_form_id'] = array(
    '#type' => 'hidden',
    '#value' => !empty($tourney->id) ? $tourney->id : $random_id,
  );

  // Used as the base url for ctools modal popup.
  $form['configure_round_names_url'] = array(
    '#type' => 'hidden',
    '#attributes' => array('class' => array('edit-configure-round-names-url')),
    '#value' => url('tourney/nojs/rounds/configure/' . $form['rounds_form_id']['#value']),
    '#weight' => -100,
  );

  // Used as the base url for ctools modal popup.
  $form['configure_seed_names_url'] = array(
    '#type' => 'hidden',
    '#attributes' => array('class' => array('edit-configure-seed-names-url')),
    '#value' => url('tourney/nojs/seeds/configure/' . $form['seeds_form_id']['#value']),
    '#weight' => -100,
  );

  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Tournament Name'),
    '#description' => t('Enter the name of this tournament.'),
    '#size' => 40,
    // Do not allow names longer than 238, to compensate for _match-00_game-00
    '#maxlength' => 238,
    '#default_value' => isset($tourney->label) ? $tourney->label : '',
    '#weight' => -10,
    '#required' => TRUE,
  );

  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($tourney->label) ? $tourney->name : '',
    // Do not allow names longer than 238, to compensate for _match-00_game-00
    '#maxlength' => 238,
    '#description' => t('A unique name for tournaments. It must only contain lowercase letters, numbers and hyphens.'),
    '#machine_name' => array(
      'source' => array('label'),
      'exists' => 'tourney_name_exists',
      'replace_pattern' => '[^a-z0-9-]+',
      'replace' => '-',
    ),
    '#element_validate' => array('tourney_machine_name_uniquify', 'form_validate_machine_name'),
    '#weight' => -10,
  );

  $form['format'] = array(
    '#type' => 'select',
    '#title' => t('Tournament style'),
    '#description' => t('Choose the style of play for this tournament'),
    '#options' => tourney_get_type_options(),
    '#disabled' => isset($tourney->format) ? TRUE : FALSE,
    '#default_value' => isset($tourney->format) ? $tourney->format : -1,
    '#attributes' => array('class' => array('edit-configure-round-names-url', 'edit-configure-seed-names-url')),
    '#weight' => -8,
  );

  // Get options form from plugin if one exists.
  tourney_get_plugin_options($tourney, $form, $form_state);
  
  $plugins_with_players = array();
  foreach ($form['plugin_options'] as $key => $plugin) {
    if (strpos($key, '#') === 0) continue;
    if (!empty($plugin['players'])) {
      $plugins_with_players[] = $key;
    }
  }
  $form['plugins_with_players'] = array(
    '#type' => 'hidden',
    '#default_value' => json_encode($plugins_with_players),
    '#attributes' => array('class' => array('edit-configure-seed-names-url')),
  );

  $form['additional_settings'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => 99,
  );

  $form['tourney'] = array(
    '#type' => 'fieldset',
    '#title' => 'Tournament Settings',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#weight' => 0,
  );
  
  $form['tourney']['config_show_byes'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show Byes'),
    '#description' => t('Show matches where the contestant advances automatically due to an uneven number of players.'),
    '#default_value' => $tourney->get('show_byes', FALSE),
  );

  $form['tourney']['config_no_automation'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do not move contestants automatically'),
    '#description' => t('If on, you will need to manually set contestants in every match. Tourney will not move contestants to next match.'),
    '#default_value' => $tourney->get('no_automation'),
  );

  $form['tourney']['config_show_round_names'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show Round Names'),
    '#description' => t('Show round names above each round.'),
    '#default_value' => $tourney->get('show_round_names', FALSE),
  );

  $form['tourney']['configure_round_names'] = array(
    '#type' => 'button',
    '#value' => t('Configure round names'),
    '#attributes' => array('class' => array('ctools-use-modal')),
    '#states' => array(
      'visible' => array(
        ':input[name="config_show_round_names"]' => array('checked' => TRUE),
      ),
    ),
  );
  
  $form['tourney']['config_show_seed_names'] = array(
    '#type' => 'checkbox',
    '#title' => t('Customize Seeds'),
    '#description' => t('Customize seed names for each contestant.'),
    '#default_value' => $tourney->get('show_seed_names', FALSE),
  );

  $form['tourney']['configure_seed_names'] = array(
    '#type' => 'button',
    '#value' => t('Configure seed names'),
    '#attributes' => array('class' => array('ctools-use-modal')),
    '#states' => array(
      'visible' => array(
        ':input[name="config_show_seed_names"]' => array('checked' => TRUE),
      ),
    ),
  );
  
  $form['extra_fields'] = array(
    '#type' => 'fieldset',
    '#title' => t('Extra fields'),
    '#description' => t('Choose which extra fields to enable.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#access' => user_access('administer tourney'),
    '#weight' => 20,
  );

  $extra_field_options = drupal_map_assoc(array('tournament', 'match_winner', 'contestants', 'match_table'));
  $form['extra_fields']['config_match_extra_fields'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Match fields'),
    '#options' => $extra_field_options,
    '#description' => t('Extra fields to show on the match entity view.'),
    '#default_value' => ($tourney->is_new) ? $extra_field_options : $tourney->get('match_extra_fields', array()),
  );

  $form['bundles'] = array(
    '#type' => 'fieldset',
    '#title' => 'Tournament Bundles',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#weight' => 10,
  );

  $form['bundles']['config_match_bundle'] = array(
    '#type' => 'select',
    '#title' => t('Match Bundle'),
    '#description' => t('Select which bundle to use for this tournament\'s matches'),
    '#options' => drupal_map_assoc(array_keys(field_info_bundles('tourney_match'))),
    '#default_value' => $tourney->get('match_bundle', 'match'),
    '#disabled' => isset($tourney->id) ? TRUE : FALSE,
  );

  $form['bundles']['config_game_bundle'] = array(
    '#type' => 'select',
    '#title' => t('Game Bundle'),
    '#description' => t('Select which bundle to use for this tournament\'s games'),
    '#options' => drupal_map_assoc(array_keys(field_info_bundles('tourney_game'))),
    '#default_value' => $tourney->get('game_bundle', 'game'),
    '#disabled' => isset($tourney->id) ? TRUE : FALSE,
  );

  $form['bundles']['config_default_games'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Number of Games'),
    '#size' => 10,
    '#description' => t('Enter the default number of games for each match.'),
    '#default_value' => $tourney->get('default_games', TOURNEY_GAMES),
    '#disabled' => isset($tourney->id) ? TRUE : FALSE,
    '#element_validate' => array('tourney_tournament_validate_games'),
  );

  $form['contestants'] = array(
    '#type' => 'fieldset',
    '#title' => t('Contestant Settings'),
    '#description' => t('Settings pertaining to contestants.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#access' => user_access('administer tourney'),
    '#weight' => 20,
  );

  $form['contestants']['config_truncate'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of characters to truncate contestant names to.'),
    '#description' => t('Names that have been truncated will have an ellipsis added to the end of the string.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => $tourney->get('truncate', TOURNEY_TRUNCATE),
  );
  
  $view_mode_options = _tourney_get_contestant_view_modes();
  $form['contestants']['config_contestant_view_mode'] = array(
    '#type' => 'select',
    '#title' => t('View mode to use when rendering a contestant "extra" field.'),
    '#options' => _tourney_get_contestant_view_modes(),
    '#description' => t('Choose any view mode available to a contestant entity.'),
    '#default_value' => $tourney->get('contestant_view_mode', 'full'),
  );

  $form['path'] = tourney_create_path_field($tourney);

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
    '#default_value' => isset($tourney->status) ? $tourney->status : 0,
  );

  $form['debug'] = array(
    '#type' => 'fieldset',
    '#title' => t('Debug'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#access' => user_access('administer tourney'),
    '#weight' => 100,
  );

  $form['debug']['config_show_next_match_path'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show next match identifier in match block'),
    '#default_value' => $tourney->get('show_next_match_path', -1),
  );

  $form['debug']['config_show_previous_match_path'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show previous match identifier in match block'),
    '#default_value' => $tourney->get('show_previous_match_path', -1),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 10,
  );

  if (!empty($tourney->id)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 40,
      '#submit' => array('tourney_tournament_delete_submit'),
    );
  }

  field_attach_form('tourney_tournament', $tourney, $form, $form_state);

  $form['#submit'][] = 'tourney_tournament_form_submit';
  return $form;
}

/**
 * Helper function for creating a dropdown of view modes for contestants.
 */
function _tourney_get_contestant_view_modes() {
  $options = array();
  
  module_load_include('inc', 'tourney', 'includes/tourney.admin');
  $entities = entity_get_info();
  foreach ($entities as $key => $entity) {
    $report = tourney_relation_type_tableselect_values('contestant', $key);
    if (!empty($report)) {
      foreach($entity['view modes'] as $vm_key => $view_mode) {
        $options[$entity['label']][$vm_key] = $view_mode['label'];
      }
    }
  }
  
  return $options;
}

/**
 * Validate the number of games.
 */
function tourney_tournament_validate_games($element, &$form_state) {
  $value = $element['#value'];
  if (!is_int($value) && $value % 2 === 0) {
    form_error($element, t('%name must be an odd number.',
      array('%name' => $element['#title'])));
  }
}

/**
 * Machine name callback to make sure it name does not already exist.
 */
function tourney_name_exists($value) {
  return db_query_range('SELECT 1 FROM {tourney_tournament} WHERE name = :name', 0, 1, array(':name' => $value))->fetchField();
}

function tourney_tournament_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $tourney = $form_state['tourney'];
  $form_state['redirect'] = array($tourney->getUri() . '/delete', array('query' => $destination));
}

/**
 * Field validation for the machine name field. Check to ensure a machine name
 * is unique and add suffix variants if necessary.
 *
 * This function will change the machine name to 'machine_name-0' and will
 * increase the number suffix until it finds a unique machine name.
 *
 * @param $element
 *   The machine name form element.
 * @param $form_state
 *   The form state from the submitted form.
 */
function tourney_machine_name_uniquify(&$element, &$form_state) {
  if (!tourney_name_exists($element['#value'])) {
    return;
  }
  // Do not allow names longer than 238, to compensate for _match-00_game-00
  $maxlength = '238';
  $separator = '-';
  $original_machine_name = $element['#value'];

  if ($original_machine_name == $form_state['values']['name']) return;

  $i = 0;
  do {
    // Append an incrementing numeric suffix until we find a unique alias.
    $unique_suffix = $separator . $i;
    $element['#value'] = truncate_utf8($original_machine_name, $maxlength - drupal_strlen($unique_suffix, TRUE)) . $unique_suffix;
    $i++;
  } while (tourney_name_exists($element['#value']));

  // Change the value in the form that was submitted to unique machine name.
  $form_state['values']['name'] = $element['#value'];
}

/**
 * Tournament form submit handler.
 */
function tourney_tournament_form_submit($form, &$form_state) {
  $tourney = $form_state['tourney'];

  // Save settings to a config array, for use before config is actually saved
  // to the tourney table.
  $tourney->config = array();
  foreach ($form_state['values'] as $key => $value) {
    if (substr($key, 0, 7) != 'config_') {
      continue;
    }
    $tourney->config[substr($key, 7)] = $value;
  }
  tourney_set_plugin_options($tourney, $form_state, TRUE);

  entity_form_submit_build_entity('tourney_tournament', $tourney, $form, $form_state);

  // Grab the number of contestants from the plugin.
  $controller = $tourney->format;
  $plugin = new $controller(0, $tourney);
  $plugin->getPluginOptions();
  if (!$players = $plugin->getNumberOfPlayers()) {
    throw new Exception('Plugin returned 0 players. Tournament can not be constructed.');
  }
  $tourney->players = $players;
  unset($plugin);
  tourney_tournament_save($tourney);

  // Any fields preceded with "config_" should be saved as configuration.
  foreach ($form_state['values'] as $key => $value) {
    if (substr($key, 0, 7) != 'config_') {
      continue;
    }
    $tourney->set(substr($key, 7), $value);
  }

  // Save plugin options
  tourney_set_plugin_options($tourney, $form_state);

  // Clear the ctools object cache
  if ($tourney->config['show_round_names']) {
    $object = tourney_configure_rounds_cache_get($form_state['values']['rounds_form_id']);
    tourney_configure_rounds_cache_set($tourney->id, $object);
    tourney_configure_rounds_cache_clear($form_state['values']['rounds_form_id']);
  }

  drupal_set_message(t('Your tournament has been saved.'));
  $form_state['redirect'] = $tourney->getUri();
}

/**
 * Show the tournament page.
 */
function tourney_tournament_page_view($tourney, $view_mode = 'full') {
  $entity_info = entity_get_info('tourney_tournament');
  $mode = array_key_exists($view_mode, $entity_info['view modes'])
    ? $view_mode : 'full';
    $tourney->tourneyFormatPlugin->build();
  return $tourney->view($mode, NULL, TRUE);
}

/**
 * Find all the games for a given match
 *
 * @param $entity
 *   A match entity to find games associated with.
 * @param $load_entity (boolean)
 *   Load the game entity and return the full object.
 * @return $games
 *   Returns an array of game objects.
 */
function tourney_tournament_get_match_games($entity, $load_entity = TRUE, $reset = FALSE) {
  $cache_name = 'tourney_match_games_' . $entity->id . ($load_entity ? '_entity' : '');
  $cache = cache_get($cache_name, 'cache_tourney');
  if ($reset || !$cache || ($cache && empty($cache->data))) {
    $query = tourney_relation_query('tourney_match', $entity->id);
    $query->entityCondition('bundle', 'has_game');
    $results = $query->execute();

    $games = array();
    foreach ($results as $id => $relation) {
      $relation = relation_load($id);
      $endpoints = field_get_items('relation', $relation, 'endpoints');

      if ($load_entity) {
        $game = tourney_game_load($endpoints[1]['entity_id']);
        $games[$game->id] = $game;
      }
      else {
        $games[$endpoints[1]['entity_id']] = $endpoints[1]['entity_id'];
      }
    }
    cache_set($cache_name, $games, 'cache_tourney');
    return $games;
  }
  return !empty($cache->data) ? $cache->data : array();
}

/**
 * Delete multiple tournament entities.
 *
 * @todo This function may be deprecated and certinally should not be called
 *       as any form of programming api as it will leave matches in the db.
 *
 * @param $ids
 *   An array of test entity IDs.
 */
// function tourney_tournament_delete_multiple(array $ids) {
//   entity_get_controller('tourney_tournament')->delete($ids);
// }

/**
 * Page callback. Show a listing of default entities.
 */
function tourney_overview_entities() {
  foreach (tourney_entities() as $entity_name => $entity_type) {
    // Entity name minus the tourney_ prefix
    $entity_short = substr($entity_name, 8);

    $label = t('Manage @entity', array('@entity' => ucfirst($entity_short)));
    $items[] = l($label, 'admin/structure/tourney/' . $entity_short);
  }
  return theme('item_list', array('items' => $items));
}

/**
 * Page callback. Edit a tournament.
 */
function tourney_tournament_edit($tournament) {
  drupal_set_title(t('Edit Tournament: @title', array('@title' => $tournament->label)));

  return drupal_get_form('tourney_tournament_form', $tournament);
}

/**
 * Build an options array to be used in forms.
 */
function tourney_get_type_options($reset = FALSE) {
  ctools_include('plugins');
  static $options = array();

  if (!empty($options) && !$reset) {
    return $options;
  }

  $plugins = tourney_get_types();
  uasort($plugins, 'ctools_plugin_sort');
  foreach ($plugins as $info) {
    $options[$info['controller']] = $info['title'];
  }
  return $options;
}

/**
 * Load all the plugin option forms if one is available.  These forms are hidden
 * via the Drupal #states system, and only shown in the UI if the tournament
 * style for the controller is selected.
 *
 * @todo Think about having these options load with ajax. It may get unwieldy if
 *   we have a ton of Tournament styles.
 */
function tourney_get_plugin_options($tourney, &$form, &$form_state) {
  $form['plugin_options'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
  );

  $plugins = tourney_get_types();
  uasort($plugins, 'ctools_plugin_sort');
  foreach ($plugins as $info) {
    $controller = ctools_plugin_get_class($info, 'controller');
    if (class_exists($controller) && method_exists($controller, 'optionsForm')) {
      $plugin = new $controller(0, $tourney);
      $options_form = $plugin->optionsForm($form_state);

      // If no options are defined move on.
      if (empty($options_form)) {
        continue;
      }

      $form['plugin_options'][$controller] = array(
        '#type' => 'fieldset',
        '#title' => t('!title Settings', array('!title' => $info['title'])),
        '#collapsible' => FALSE,
        '#access' => user_access('administer tourney'),
        '#tree' => TRUE,
        '#states' => array(
          'visible' => array(
            ':input[name="format"]' => array('value' => $controller),
          ),
        ),
      ) + $options_form;
    }
  }
}

/**
 * Save any options the plugin has set
 *
 * @param $tourney
 *   The tourney object to save settings for.
 * @param $form_state
 *   The tourney form.
 * @param $to_cache
 *   If a tournament hasn't been saved yet, use this flag to store setting to cache.
 * 
 * @todo: refactor this duplicate code with seeds and rounds.
 */
function tourney_set_plugin_options(&$tourney, &$form_state, $to_cache = FALSE) {
  module_load_include('inc', 'tourney', 'includes/tourney.modal.rounds');
  module_load_include('inc', 'tourney', 'includes/tourney.modal.seeds');
  
  // Add the round names to tourney settings
  if ($tourney->config['show_round_names']) {
    $rounds = tourney_configure_rounds_cache_get($form_state['values']['rounds_form_id']);
    foreach ((array) $rounds as $key => $round) {
      if (strpos($key, 'round_') === 0) {
        $form_state['values']['plugin_options'][$key] = $round;
      }
    }
  }
  
  // Add the seed names to tourney settings
  if ($tourney->config['show_seed_names']) {
    $seeds = tourney_configure_seeds_cache_get($form_state['values']['seeds_form_id']);
    foreach ((array) $seeds as $key => $seed) {
      if (strpos($key, 'seed_') === 0) {
        $form_state['values']['plugin_options'][$key] = $seed;
      }
    }
  }

  if (!empty($form_state['values']['plugin_options'])) {
    foreach ($form_state['values']['plugin_options'] as $name => $values) {
      if ($to_cache == TRUE) {
        $tourney->config[$name] = $values;
      }
      else {
        $tourney->set($name, $values);
      }
    }
  }
}
