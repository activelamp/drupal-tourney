<?php

/**
 * @file
 * Provides the page callbacks for rounds ctools modal.
 */


function tourney_configure_rounds($js = FALSE, $object_id, $controller, $slots) {
  // @todo: Make these player counts not hard coded
  list($single, $double, $roundrobin) = array_slice(func_get_args(), -3);
  switch($controller) {
    case 'SingleEliminationController': $slots = $single; break;
    case 'DoubleEliminationController': $slots = $double; break;
    case 'RoundRobinController':        $slots = $roundrobin; break;
  }
  
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

