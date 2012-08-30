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
  $form['rounds_form_id'] = array(
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
    '#attributes' => array('class' => array('edit-configure-round-names-url')),
    '#weight' => -8,
  );     
  
  // Get options form from plugin if one exists.
  tourney_get_plugin_options($tourney, $form, $form_state);

  $form['players'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of contestants'),
    '#description' => t('Enter the number contestants playing in this tournament. Must be at least 4 or more players.'),
    '#size' => 10,
    '#required' => TRUE,
    '#maxlength' => 2,
    '#disabled' => isset($tourney->players) ? TRUE : FALSE,
    '#default_value' => isset($tourney->players) ? $tourney->players : '',
    '#attributes' => array('class' => array('edit-configure-round-names-url')),
    '#weight' => 1,
    '#element_validate' => array('tourney_tournament_validate_contestants'),
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
    '#default_value' => $tourney->get('config_truncate', TOURNEY_TRUNCATE),
  );
  

  $path = array();
  if (!empty($tourney->entity_id)) {
    $conditions = array('source' => 'tourney/tournament/' . $tourney->entity_id);
    $path = path_load($conditions);
    if ($path === FALSE) {
      $path = array();
    }
  }

  $form['path'] = array(
    '#type' => 'fieldset', 
    '#title' => t('URL Path Settings'), 
    '#collapsible' => TRUE, 
    '#collapsed' => empty($path['alias']), 
    '#group' => 'additional_settings', 
    '#attributes' => array(
      'class' => array('path-form'),
    ), 
    '#attached' => array(
      'js' => array(drupal_get_path('module', 'path') . '/path.js'),
    ), 
    '#access' => user_access('create url aliases') || user_access('administer url aliases'), 
    '#weight' => 30, 
    '#tree' => TRUE, 
    '#element_validate' => array('path_form_element_validate'),
  );

  $path += array(
    'pid' => NULL, 
    'source' => isset($tourney->entity_id) ? 'tourney/tournament/' . $tourney->entity_id : NULL, 
    'alias' => '', 
    'language' => isset($tourney->language) ? $tourney->language : LANGUAGE_NONE,
  );

  $form['path']['alias'] = array(
    '#type' => 'textfield', 
    '#title' => t('URL alias'), 
    '#default_value' => $path['alias'], 
    '#maxlength' => 255, 
    '#collapsible' => TRUE, 
    '#collapsed' => TRUE, 
    '#description' => t('Optionally specify an alternative URL by which this content can be accessed. For example, type "about" when writing an about page. Use a relative path and don\'t add a trailing slash or the URL alias won\'t work.'),
  );
  $form['path']['pid'] = array(
    '#type' => 'value',
    '#value' => $path['pid'],
  );
  $form['path']['source'] = array(
    '#type' => 'value',
    '#value' => $path['source'],
  );
  $form['path']['language'] = array(
    '#type' => 'value',
    '#value' => $path['language'],
  );

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

  field_attach_form('tourney_tournament', $tourney, $form, $form_state);

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

  $form['#submit'][] = 'tourney_tournament_form_submit';

  return $form;
}

/**
 * Validate the number of contestants. Needs to be a number greater than 3
 */
function tourney_tournament_validate_contestants($element, &$form_state) {
  $value = $element['#value'];
  if ($value !== '' && (!is_numeric($value) || intval($value) != $value || $value < 3)) {
    form_error($element, t('%name must be four or more.', array('%name' => $element['#title'])));
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
function tourney_tournament_page_view($tourney) {
  return $tourney->view('full', NULL, TRUE);
}

/**
 * Implements hook_ENTITY_view().
 */
function tourney_tourney_tournament_view($entity, $type, $view_mode, $langcode = 'en') {
  $info = TourneyController::getPluginInfo($entity->format);
  
  $entity->content['#attached']['css'] = array(
    drupal_get_path('module', 'tourney') . '/theme/tourney.css'
  );
  $entity->content['#attached']['js'] = array(
    drupal_get_path('module', 'tourney') . '/theme/tourney.js'
  );
  $entity->content['tourney_format'] = array(
   '#prefix' => '<div class="field-label">Tournament Format:</div><div class="field-content">',
   '#suffix' => '</div>',
   '#markup' => $info['title'],
  );
  $entity->content['tourney_players'] = array(
   '#prefix' => '<div class="field-label">Number of players:</div><div class="field-content">',
   '#suffix' => '</div>',
   '#markup' => $entity->players,
  );
  $entity->content['tourney_winner'] = array(
   '#prefix' => '<div class="field-label">Winner:</div><div class="field-content">',
   '#suffix' => '</div>',
   '#markup' => $entity->winner,
  );
  // Add the matches for this tournament
  $entity->content['matches'] = array(
    '#prefix' => '<h2>Matches in this tournament</h2>',
    '#markup' => $entity->render(),
  );
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
    $query = relation_query('tourney_match', $entity->id);
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
 * @param $ids
 *   An array of test entity IDs.
 */
function tourney_tournament_delete_multiple(array $ids) {
  entity_get_controller('tourney_tournament')->delete($ids);
}

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
 */
function tourney_set_plugin_options(&$tourney, &$form_state, $to_cache = FALSE) {
  // Add the round names to tourney settings
  if ($tourney->config['show_round_names']) {
    $rounds = tourney_configure_rounds_cache_get($form_state['values']['rounds_form_id']);
    foreach ((array) $rounds as $key => $round) {
      if (strpos($key, 'round_') === 0) {
        $form_state['values']['plugin_options'][$key] = $round;
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

function tourney_configure_rounds($js = FALSE, $object_id, $slots, $controller) {
  $form_state = array(
    'ajax' => $js,
    'title' => t('Configure Round names.'),
    'controller' => $controller,
    'slots' => $slots,
    'object_id' => $object_id,
  );
  
  if ($js) {
    ctools_include('ajax');
    ctools_include('modal');
    $output = ctools_modal_form_wrapper('tourney_configure_rounds_form', $form_state);
    
    if ($form_state['executed'] && $js) {
      $commands = array();
      $commands[] = ctools_modal_command_dismiss();
      
      print ajax_render($commands);
      exit;
    }

    print ajax_render($output);
    exit;
  }
  else {
    return drupal_get_form('tourney_configure_rounds_form', $form_state, $form_state);
  }
}

function tourney_configure_rounds_form($form, &$form_state) {
  if (!array_key_exists('controller', $form_state) && $form_state['build_info']['args'][0]['controller']) {
    $form_state['controller'] = $form_state['build_info']['args'][0]['controller'];
    $form_state['slots'] = $form_state['build_info']['args'][0]['slots'];
    $form_state['object_id'] = $form_state['build_info']['args'][0]['object_id'];
  }
  
  // Instantiate a tournament so we can look at the data array.
  $plugin = new $form_state['controller']($form_state['slots']);
  $plugin->build();
  
  $form_state['object'] = tourney_configure_rounds_cache_get($form_state['object_id']);
  
  $form['num_rounds'] = array(
    '#type' => 'value',
    '#value' => count($plugin->data['rounds']),
  );
  
  foreach ($plugin->data['rounds'] as $rid => $round) {
    // Set the bracket fieldset
    if (!array_key_exists('bracket_' . $round['bracket'], $form)) {
      $form['bracket_' . $round['bracket']] = array(
        '#type' => 'fieldset',
        '#title' => t('%bracket_name Bracket', array('%bracket_name' => $round['bracket'])),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
        '#access' => user_access('administer tourney'),
      );
    }
    
    // If the object_id is numeric, we have a tournament id.
    if (is_numeric($form_state['object_id'])) {
      $tourney = tourney_tournament_load($form_state['object_id']);
      $round_title = $tourney->get('round_' . $rid, $round['title']);
    }
    else {
      $round_title = $round['title'];
    }
    
    $property = 'round_' . $rid;
    $form['bracket_' . $round['bracket']]['round_' . $rid] = array(
      '#type' => 'textfield',
      '#title' => $round['title'],
      '#description' => t('Enter the title for %round_name in the %bracket_name bracket.', array('%round_name' => $round['title'], '%bracket_name' => $round['bracket'])),
      '#size' => 40,
      '#maxlength' => 255,
      '#default_value' => property_exists($form_state['object'], $property) ? $form_state['object']->$property : $round_title,
    );
  }
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Set Round Names'),
  );
  
  return $form;
}

/**
 * Set the round names in object cache.
 */
function tourney_configure_rounds_form_submit(&$form, &$form_state) {
  $form_state['object']->num_rounds = $form_state['values']['num_rounds'];
  
  for($i = 1; $i <= $form_state['object']->num_rounds; $i++) {
    $property = 'round_' . $i;
    $form_state['object']->$property = $form_state['values']['round_' . $i];
  }
  tourney_configure_rounds_cache_set($form_state['object_id'], $form_state['object']);
}

/**
 * Store our little cache so that we can retain data from form to form.
 */
function tourney_configure_rounds_cache_set($id, $object) {
  ctools_include('object-cache');
  ctools_object_cache_set('tourney_configure_rounds', $id, $object);
}

/**
 * Get the current object from the cache, or default.
 */
function tourney_configure_rounds_cache_get($id) {
  ctools_include('object-cache');
  $object = ctools_object_cache_get('tourney_configure_rounds', $id);
  if (!$object) {
    // Create a default object.
    $object = new stdClass;
  }

  return $object;
}

/**
 * Clear the cache.
 */
function tourney_configure_rounds_cache_clear($id) {
  ctools_include('object-cache');
  ctools_object_cache_clear('tourney_configure_rounds', $id);
}