<?php

/**
 * @file
 * Adds tourament pages.
 */


/**
 * The tournament form.
 */
function tourney_tournament_form($form, &$form_state) {
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Tournament Name'),
    '#description' => t('Enter the name of this tournament.'),
    '#size' => 40,
    '#maxlength' => 255,
  );
  
  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Tournament style'),
    '#description' => t('Choose the style of play for this tournament'),
    '#options' => drupal_map_assoc(array('Single Elimination', 'Double Elimination', 'Round Robin', 'Ladder')),
    '#default_value' => -1,
  );
  
  $form['players'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of contestants'),
    '#description' => t('Enter the number contestants playing in this tournament'),
    '#size' => 10,
    '#maxlength' => 2,
  );
  
  $form['winner'] = array(
    '#type' => 'select',
    '#title' => t('Winner'),
    '#description' => t('The description appears usually below the item.'),
    '#options' => drupal_map_assoc(array('No Winner')),
    '#default_value' => -1,
  );
  
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
  );

  $form['#submit'][] = 'tourney_tournament_form_submit';
  return $form;
}
/**
 * Tournament form submit handler.
 */
function tourney_tournament_form_submit($form, &$form_state) {
  drupal_set_message(t('Your tournament has been saved.'));
  
  $tourney = new stdClass();
  entity_form_submit_build_entity('tourney_tournament', $tourney, $form, $form_state);
  entity_save('tourney_tournament', $tourney);  
}

/**
 * Show the tournament page.
 */
function tourney_tournament_page_view($tourney) {
  return $tourney->view('full', 'en', 'page');
}

/**
 * Implements hook_ENTITY_view().
 */
function tourney_tourney_tournament_view($entity, $type, $view_mode, $langcode = 'en') {
  $entity->content = array(
    'type' => array(
      '#markup' => $entity->type,
    ),
    'players' => array(
      '#markup' => $entity->players,
    ),
  );
}

/**
 * Delete multiple tournament entities.
 *
 * @param $tids
 *   An array of test entity IDs.
 */
function tourney_tournament_delete_multiple(array $tids) {
  entity_get_controller('tourney_tournament')->delete($tids);
}