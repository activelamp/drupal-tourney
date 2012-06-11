<?php

/**
 * @file
 * Tourney type editing UI.
 */

/**
 * Generates the profile type editing form.
 */
function tourney_type_form($form, &$form_state, $tourney_type, $op = 'edit') {
  if ($op == 'clone') {
    $tourney_type->label .= ' (cloned)';
    $tourney_type->type = '';
  }

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => isset($tourney_type->label) ? $tourney_type->label : '',
    '#description' => t('The human-readable name of this type.'),
    '#required' => TRUE,
    '#size' => 30,
  );
  // Machine-readable type name.
  $form['type'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($tourney_type->type) ? $tourney_type->type : '',
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => $form_state['entity_type'] . '_get_types',
      'source' => array('label'),
    ),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save tournament type'),
    '#weight' => 40,
  );

  if ($op != 'add' && $op != 'clone') {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete tournament type'),
      '#weight' => 45,
      '#limit_validation_errors' => array(),
      '#submit' => array('tourney_tournament_type_form_submit_delete')
    );
  }

  return $form;
}

/**
 * Form API submit callback for the type form.
 */
function tourney_tournament_type_form_submit(&$form, &$form_state) {
  $tourney_tournament_type = entity_ui_form_submit_build_entity($form, $form_state);
  // Save and go back.
  $tourney_tournament_type->save();
  $form_state['redirect'] = 'admin/structure/tournament';
}

/**
 * Form API submit callback for the delete button.
 */
function tourney_tournament_type_form_submit_delete(&$form, &$form_state) {
  drupal_set_message(t('The tournament and all its matches and games have been deleted.'));
  $form_state['redirect'] = 'admin/structure/tournament/manage/' . $form_state['tourney_tournament_type']->type . '/delete';
}

/**
 * Page callback.
 */
function tourney_admin_content() {
  $output = '';
  $header = array(
    'id' => array('data' => t('Id'), 'field' => 't.id', 'sort' => 'asc'),
    'title' => array('data' => t('Label'), 'field' => 't.label'),
    'format' => array('data' => t('Format'), 'field' => 't.format'),
    'players' => array('data' => t('Players'), 'field' => 't.players'),
    'winner' => array('data' => t('Winner'), 'field' => 't.winner'),
    'operations' => array('data' => t('Operations'), 'colspan' => '3'),
  );

  // Grab all tournaments.
  $query = db_select('tourney_tournament', 't')->extend('PagerDefault')->extend('TableSort');
  $query->fields('t', array('id', 'label', 'format', 'players', 'winner'))
    ->limit(50)
    ->orderByHeader($header);
  $tournaments = $query->execute();

  $output .= theme('item_list', array('items' => array(l(t('Add tournament'), 'tourney/add')), 'attributes' => array('class' => 'action-links')));
  $output .= theme('tourney_admin_content', array('tournaments' => $tournaments, 'header' => $header));
  return $output;
}

/**
 * Generate a table of all tournaments on this site.
 */
function theme_tourney_admin_content($variables) {
  module_load_include('php', 'tourney', 'plugins/tourney_formats/BaseFormatControllers/SpecialElimination');
  $tournament = new SpecialEliminationController(6);
  krumo($tournament);

  $tournaments = $variables['tournaments'];
  $header = $variables['header'];

  $rows = array();
  if ( empty($tournaments) ) {
    $message = t('There are currently no tournaments on your site.');

    $rows[] = array(
      array('data' => $message, 'colspan' => 8),
    );
  }
  else foreach ($tournaments as $tournament) {
    // $tournament is not a fully loaded object. Need to get the path via the
    // static method on tournaments.
    $uri = TourneyTournament::path($tournament->id);
    $info = TourneyController::getPluginInfo($tournament->format);
    $rows[] = array(
      l($tournament->id, $uri),
      l($tournament->label, $uri),
      $info['name'],
      $tournament->players,
      $tournament->winner,
      user_access('access tourney') ? l(t('View'), $uri . '') : '',
      user_access('edit tourney') ? l(t('Edit'), $uri . '/edit') : '',
      user_access('delete tourney') ? l(t('Delete'), $uri . '/delete') : '',
    );
  }

  return theme('table', array('header' => $header, 'rows' => $rows)) . theme('pager');
}

function tourney_confirm_delete_page($tournament) {
  if ($tournament) {
    return drupal_get_form('tourney_confirm_delete', $tournament);
  }
  return MENU_NOT_FOUND;
}

function tourney_confirm_delete($form, &$form_state, $tournament) {
  $form['#tournament'] = $tournament;
  $form['#tid'] = $tournament->id;
  return confirm_form(
    $form,
    t('Are you sure you want to delete the tournament %title?', array('%title' => $tournament->label)),
    'tourney/' . $tournament->id,
    t('All matches belonging to this tournament will be deleted. This action cannot be undone.'),
    t('Delete'),
    t('Cancel'),
    'tourney_confirm_delete');
}

/**
 * Submit handler after confirming deletion.
 */
function tourney_confirm_delete_submit($form, &$form_state) {
  // Delete the tournament.
  $form['#tournament']->delete();
  $form_state['redirect'] = "admin/content/tourney";
}

/**
 * Tourney admin configuration settings form.
 *
 * This form will allow individual selection of 'bundles' elements for
 * every entity type reported by entity_get_info().
 */
function tourney_configuration($form, &$form_state) {
  // Capture enumeration of content types.
  $entities = entity_get_info();
  $filter_entities = array('tourney_tournament', 'tourney_match', 'tourney_game', 'relation', 'rules_config', 'comment');
  foreach ( $entities as $key => $entity ) {
    if ( $entity['controller class'] == 'EntityAPIControllerExportable' ) 
      unset($entities[$key]);
  }
  foreach ( $filter_entities as $filter ) {
    unset($entities[$filter]);
  }

  $header = array(
    'bundle_name' => t('Bundle Name'),
    // @todo: It would be nice to be able to configure what field you want to
    // use for the contestant name.
    // 'label_field' => t('Contestant Name Field'),
  );

  $form['tourney_header'] = array(
    '#prefix' => '<h3>' . t('Tournament Module Configuration') . '</h3><div>',
    '#suffix' => '</div>',
  );
  $form['tourney_debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('Turn debug mode on'),
    '#default_value' => variable_get('tourney_debug', FALSE),
  );
  $form['tourney_backward_compatibility'] = array(
    '#type' => 'checkbox',
    '#title' => t('Backward Compatibility'),
    '#default_value' => variable_get('tourney_backward_compatibility', FALSE),
    '#description' => t('Tourney objects used to have properties available right on the object. There are now getter methods in place to get these properties. Check this box if you have code accessing these properties directly and still need to the data directly on the object.'),
  );
  
  
  $form['tourney_games'] = array(
    '#type' => 'textfield',
    '#title' => t('Games'),
    '#description' => t('Number of games per match'),
    '#default_value' => variable_get('tourney_games_all', TOURNEY_GAMES),
    '#size' => 2,
    '#maxlength' => 2,
    '#required' => TRUE,
  );
  
  $form['tourney'] = array(
    '#prefix' => t('Select the entity bundles that can compete.'),
    '#type' => 'vertical_tabs',
  );
  $form['tourney']['#attached']['css'] = array(
    drupal_get_path('module', 'tourney') . '/theme/tourney.css',
  );

  foreach ($entities as $key => $entity) {
    $form['tourney'][$key] = array(
      '#type' => 'fieldset',
      '#title' => $entity['label'],
      '#weight' => 10,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['tourney'][$key]['tourney_type_' . $key . '_bundle'] = array(
      '#type' => 'tableselect',
      '#header' => $header,
      '#options' => tourney_relation_type_tableselect_options($key),
      '#default_value' => tourney_relation_type_tableselect_values('contestant', $key),
    );
  }
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Settings'),
    '#submit' => array('tourney_configuration_form_submit'),
    '#weight' => 10,
  );
  $form['actions']['tourney_default'] = array(
    '#type' => 'submit',
    '#value' => 'Set to Defaults',
    '#submit' => array('tourney_configuration_defaults_submit'),
    '#weight' => 20,
  );

  return $form;
}

/**
 * Submit handler for configuration settings form.
 *
 * Modify target bundles of the relation types defined by TOURNEY_CONTESTANT_1
 * and TOURNEY_CONTESTANT_2. All target bundles will be removed before being
 * recreated. A target bundle, enumerated in an entity's 'bundles' key, must
 * exist to be targeted.
 */
function tourney_configuration_form_submit($form, &$form_state) {
  $relation_types = array('contestant');
  $entities = entity_get_info();
  $targets = array();

  try {
    // Save settings to variables table
    variable_set('tourney_games_all', $form_state['values']['tourney_games']);
    variable_set('tourney_debug', $form_state['values']['tourney_debug']);
    variable_set('tourney_backward_compatibility', $form_state['values']['tourney_backward_compatibility']);
    
    // Set the relations.
    tourney_relation_type_untarget($relation_types);
    foreach ($entities as $key => $entity) {
      foreach ($entity['bundles'] as $b_key => $bundle) {
        $targets[] = $key . ':' . $b_key;
        $bit = $form_state['values']['tourney_type_' . $key . '_bundle'][$b_key];
        if ($bit) {
          tourney_relation_type_target($relation_types, $key, $b_key);
        }
      }
    }
    drupal_set_message(t('Configuration settings have been saved.'), 'status');
  }
  catch (Exception $e) {
    drupal_set_message(t('Configuration settings not saved: @msg', array('@msg' => $e->getMessage())), 'error');
  }

  return;
}

/**
 * Submit handler for reset button on configuration settings form.
 *
 * Removes all target bundles for the relation types defined by
 * TOURNEY_CONTESTANT_1 and TOURNEY_CONTESTANT_2, then creates the default
 * target bundles of user:* and node:*.
 */
function tourney_configuration_defaults_submit($form, &$form_state) {
  $relation_types = array('contestant');

  try {
    tourney_relation_type_untarget($relation_types);
    tourney_relation_type_target($relation_types, 'user', '*');
    tourney_relation_type_target($relation_types, 'node', '*');
    variable_set('tourney_games_all', TOURNEY_GAMES);
    drupal_set_message(t('Default values have been restored.'), 'status');
  }
  catch (Exception $e) {
    drupal_set_message(t('Default values can not be restored: @msg', array('@ms' => $e->getMessage())), 'error');
  }

  return $form;
}

/**
 * Setup #options for a tableselect based on all bundles of specified entity.
 *
 * @param string $entity_name
 *   Machine name of the entity type.
 *
 * @return array
 *   Array of strings (bundle label) keyed from entity bundles key.
 */
function tourney_relation_type_tableselect_options($entity_name) {
  $report = array();
  $entity = entity_get_info($entity_name);

  foreach ($entity['bundles'] as $key => $bundle) {
    $report[$key] = array(
      'bundle_name' => $bundle['label'],
      // 'label_field' => t('Label Field'),
    );
  }

  return $report;
}

/**
 * Setup #default_value for a tableselect based on relation type target
 * bundles for the specified entity.
 *
 * @param string $type
 *   Machine name of the relation type.
 * @param string $entity_name
 *   Machine name of the entity type.
 *
 * @return array
 *   Array of bool keyed from entity bundles.
 */
function tourney_relation_type_tableselect_values($type, $entity_name) {
  $report = array();
  $entity = entity_get_info($entity_name);

  foreach ($entity['bundles'] as $key => $bundle) {
    $bit = tourney_relation_type_is_target($type, $entity_name, $key);
    $bit_all = tourney_relation_type_is_target($type, $entity_name, '*');
    if ($bit || $bit_all) {
      $report[$key] = 1;
    }
  }

  return $report;
}

/**
 * Sets an entity bundle as a target for the given relation type.
 *
 * @param string|array $type
 *   Machine name of the relation type.
 * @param string $target_entity
 *   Machine name of the target entity.
 * @param string $target_bundle
 *   (optional) Machine name of the target entity bundle.
 *
 * @return bool
 *   Return FALSE on failure, or TRUE if entity bundle is targeted.
 */
function tourney_relation_type_target($type, $target_entity, $target_bundle = '*') {
  // Recursive call when $type is an array.
  if (is_array($type)) {
    foreach ($type as $type_element) {
      tourney_relation_type_target($type_element, $target_entity, $target_bundle);
    }
    return TRUE;
  }
  elseif (!isset($type) || !is_string($type)) {
    throw new Exception(t('Required paramter missing'));
  }

  try {
    $relation_type = relation_type_load($type);
    $entity_type = entity_get_info($target_entity);
  }
  catch (Exception $e) {
    throw new Exception(t('Parameter will not load object') . ': ' . $e->getMessage());
  }

  if (!$relation_type) {
    throw new Exception(t('Relation type does not exist') . ': ' . $type);
  }
  if (!$target_entity || !$entity_type) {
    throw new Exception(t('Target entity type does not exist') . ': ' . $target_entity);
  }
  if (!array_key_exists('bundles', $entity_type)) {
    throw new Exception(t('Target entity type has no bundles'));
  }
  if (!array_key_exists($target_bundle, $entity_type['bundles']) && $target_bundle != '*') {
    throw new Exception(t('Target Bundle does not exist') . $target_bundle);
  }

  $target_value = $target_entity . ':' . $target_bundle;
  // Do not duplicate values. The Relation module relies on it's select form
  // to clean itself of duplicate values. Observe how $bundles is constructed
  // in relation_type_form().
  if (!in_array($target_value, $relation_type->target_bundles)) {
    $relation_type->target_bundles[$target_value] = $target_value;
    try {
      relation_type_save($relation_type, array('relation_type'));
    }
    catch (Exception $e) {
      throw new Exception(t('Bundle could not be targeted') . ': ' . $e->getMessage());
    }
  }

  return TRUE;
}

/**
 * Removes an entity bundle as a target for the given relation type.
 *
 * @param string|array $type
 *   Machine name of the relation type.
 * @param string $target_entity
 *   (optional) Machine name of the target entity. If no entity is specified
 *   then all bundles of any entity will be removed as targets (no more
 *   targets).
 * @param string $target_bundle
 *   (optional) Machine name of the target entity bundle to remove. If no
 *   bundle is specified then all target bundles of type $target_entity will
 *   be removed.
 *
 * @return bool
 *   Return FALSE on failure, or TRUE if entity bundle is removed.
 */
function tourney_relation_type_untarget($type, $target_entity = '', $target_bundle = '') {
  // Recursive call when $type is an array.
  if (isset($type) && is_array($type)) {
    foreach ($type as $type_element) {
      tourney_relation_type_untarget($type_element, $target_entity, $target_bundle);
    }
    return TRUE;
  }
  elseif (!isset($type) || !is_string($type)) {
    throw new Exception(t('Required parameter missing'));
  }

  try {
    $relation_type = relation_type_load($type);
    $entity_type = entity_get_info($target_entity);
  }
  catch (Exception $e) {
    throw new Exception(t('Parameter will not load object') . ': ' . $e->getMessage());
  }

  if (!$relation_type) {
    throw new Exception(t('Relation type does not exist') . ': ' . $type);
  }
  if ($target_entity && !$entity_type) {
    throw new Exception(t('Target entity type does not exist') . ': ' . $target_entity);
  }
  if ($target_bundle && !$target_entity) {
    throw new Exception(t('Target bundle can not be specified without a target entity') . ': ' . $target_bundle);
  }

  $target_value = $target_entity;
  if ($target_bundle) {
    $target_value = $target_value . ':' . $target_bundle;
  }

  $targets = $relation_type->target_bundles;
  $new_targets = array();
  foreach ($targets as $key => $target) {
    $target_elm = explode(':', $target);
    if (count($target_elm) < 2) {
      throw new Exception(t('Impossible'));
    }
    if (($target_entity) && ($target_bundle)) {
      if ($target != $target_value) {
        $new_targets[$target] = $target;
      }
    }
    elseif (($target_entity) && ($target_elm[0] != $target_value)) {
      $new_targets[$target] = $target;
    }
  }
  $relation_type->target_bundles = $new_targets;

  try {
    relation_type_save($relation_type, array('relation_type'));
  }
  catch (Exception $e) {
    throw new Exception(t('Target could not be removed') . ': ' . $e->getMessage());
  }

  return TRUE;
}

/**
 * Removes all target_bundles indexes that can be simplified into a single
 * index containing entity type with wildcard, and inserts the wildcard
 * reference.
 *
 * @todo Write actual code.
 *
 * @param string|array $type
 *   Machine name of the relation type.
 *
 * Conceptually both these indexes will be removed:
 * @code
 *   your_relation_type['target_bundles'][0] = 'node:page'
 *   your_relation_type['target_bundles'][1] = 'node:article'
 * @endcode
 *
 * And the following will be added:
 * @code
 *   your_relation_type['target_bundles'][] = 'node:*'
 * @endcode
 *
 * @return bool
 *   TRUE if a simplification was performed, FALSE if not.
 */
function tourney_relation_type_simplify($type) {
  // Recursive call when $type is an array.
  if (is_array($type)) {
    foreach ($type as $type_element) {
      tourney_relation_type_simplify($type_element);
    }
    return TRUE;
  }
  elseif (!isset($type) || !is_string($type)) {
    throw new Exception(t('Required parameter missing'));
  }

  try {
    $relation_type = relation_type_load($type);
  }
  catch (Exception $e) {
    throw new Exception(t('Parameter will not load object') . ': ' . $e->getMessage());
  }

  if (!$relation_type) {
    throw new Exception(t('Relation type does not exist') . ': ' . $type);
  }

  return FALSE;
}

/**
 * Reports if an entity bundle is a target for the given relation type.
 *
 * @param string|array $type
 *   Machine name of the relation type.
 * @param string $target_entity
 *   (optional) Machine name of the target entity. If no entity is specified
 *   then all bundles of any entity, if any, will report TRUE.
 * @param string $target_bundle
 *   (optional) Machine name of the target entity bundle to match. If no
 *   bundle is specified then any target bundles of type $target_entity will
 *   report TRUE.
 *
 * @return bool
 * Return FALSE on no match, or TRUE if entity bundle is a target.
 */
function tourney_relation_type_is_target($type, $target_entity = '', $target_bundle = '') {
  $report = FALSE;

  // Recursive call when $type is an array.
  if (is_array($type)) {
    foreach ($type as $type_element) {
      tourney_relation_type_is_target($type_element, $target_entity, $target_bundle);
    }
    return TRUE;
  }
  elseif (!isset($type) || !is_string($type)) {
    throw new Exception(t('Required parameter missing'));
  }

  try {
    $relation_type = relation_type_load($type);
    $entity_type = entity_get_info($target_entity);
  }
  catch (Exception $e) {
    throw new Exception(t('Parameter will not load object') . ': ' . $e->getMessage());
  }

  if (!$relation_type) {
    throw new Exception(t('Relation type does not exist') . ': ' . $type);
  }
  if ($target_entity && !$entity_type) {
    throw new Exception(t('Target entity type does not exist') . ': ' . $target_entity);
  }
  if ($target_bundle && !$target_entity) {
    throw new Exception(t('Target bundle can not be specified without a target entity') . ': ' . $target_bundle);
  }

  $target_value = $target_entity;
  if ($target_bundle) {
    $target_value = $target_value . ':' . $target_bundle;
  }

  $targets = $relation_type->target_bundles;
  $new_targets = array();
  foreach ($targets as $key => $target) {
    $target_elm = explode(':', $target);
    if (count($target_elm) < 2) {
      throw new Exception(t('Impossible'));
    }
    if (($target_entity) && ($target_bundle)) {
      if ($target == $target_value) {
        $report = TRUE;
        break;
      }
    }
    elseif (($target_entity) && ($target_elm[0] == $target_value)) {
      $report = TRUE;
    }
  }

  return $report;
}
