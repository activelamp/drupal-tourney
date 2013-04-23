<?php

/**
 * @file
 * Provides the page callbacks for seeds ctools modal.
 * 
 * @todo: refactor this duplicate code.
 */


function tourney_configure_seeds($js = FALSE, $object_id, $slots) {
  $form_state = array(
    'ajax' => $js,
    'title' => t('Configure seed names.'),
    'slots' => $slots,
    'object_id' => $object_id,
  );

  if ($js) {
    ctools_include('ajax');
    ctools_include('modal');
    $output = ctools_modal_form_wrapper('tourney_configure_seeds_form', $form_state);

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
    return drupal_get_form('tourney_configure_seeds_form', $form_state, $form_state);
  }
}

function tourney_configure_seeds_form($form, &$form_state) {
  if (!array_key_exists('slots', $form_state) && $form_state['build_info']['args'][0]['slots']) {
    $form_state['slots'] = $form_state['build_info']['args'][0]['slots'];
    $form_state['object_id'] = $form_state['build_info']['args'][0]['object_id'];
  }

  $form_state['object'] = tourney_configure_seeds_cache_get($form_state['object_id']);

  $form['num_seeds'] = array(
    '#type' => 'value',
    '#value' => $form_state['slots'],
  );

  foreach (range(1, $form_state['slots']) as $sid) {
    $property = 'seed_' . $sid;
    $seed_title = '';
    // If the object_id is numeric, we have a tournament id.
    if (is_numeric($form_state['object_id'])) {
      $tourney = tourney_tournament_load($form_state['object_id']);
      $seed_title = $tourney->get('seed_' . $sid, $seed['title']);
    }
    if (!$seed_title) {
      $form_state['object']->$property = $sid;
    }

    $form['seed_' . $sid] = array(
      '#type' => 'textfield',
      '#title' => t('Seed #@num', array('@num' => $sid)),
      '#description' => t('Enter the title for seed in position %seed_number.', array('%seed_number' => $sid)),
      '#size' => 40,
      '#maxlength' => 255,
      '#default_value' => property_exists($form_state['object'], $property) ? $form_state['object']->$property : $seed_title,
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Set Seed Names'),
  );

  return $form;
}

/**
 * Set the seed names in object cache.
 */
function tourney_configure_seeds_form_submit(&$form, &$form_state) {
  $form_state['object']->num_seeds = $form_state['values']['num_seeds'];

  for($i = 1; $i <= $form_state['object']->num_seeds; $i++) {
    $property = 'seed_' . $i;
    $form_state['object']->$property = $form_state['values']['seed_' . $i];
  }
  tourney_configure_seeds_cache_set($form_state['object_id'], $form_state['object']);
}

/**
 * Store our little cache so that we can retain data from form to form.
 */
function tourney_configure_seeds_cache_set($id, $object) {
  ctools_include('object-cache');
  ctools_object_cache_set('tourney_configure_seeds', $id, $object);
}

/**
 * Get the current object from the cache, or default.
 */
function tourney_configure_seeds_cache_get($id) {
  ctools_include('object-cache');
  $object = ctools_object_cache_get('tourney_configure_seeds', $id);
  if (!$object) {
    // Create a default object.
    $object = new stdClass;
  }

  return $object;
}

/**
 * Clear the cache.
 */
function tourney_configure_seeds_cache_clear($id) {
  ctools_include('object-cache');
  ctools_object_cache_clear('tourney_configure_seeds', $id);
}

