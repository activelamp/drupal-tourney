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
  drupal_set_title(t('Edit Match: @title', array('@title' => $match->title)));
  
  return drupal_get_form('tourney_match_form', $match);
}

/**
 * The match form.
 */
function tourney_match_form($form, &$form_state, $tourney) {
  $tourney->type = 'match';
  $form_state['tourney'] = $tourney;
  
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Match Name'),
    '#description' => t('Enter the name of this match.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => isset($tourney->title) ? $tourney->title : '',
  );
  
  // @todo: It would be nice if these were field api fields
  if ($tourney->previous_match1 == 'manual') {
    $user = relation_get_related_entity('tourney_match', $tourney->id, 'contestant_1');
    $form['contestant1'] = array(
      '#title' => t('Contestant 1'),
      '#default_value' => is_object($user) && $user->uid ? $user->uid : -1,
      '#disabled' => is_object($user) && $user->uid ? TRUE : FALSE,
      '#required' => is_object($user) && $user->uid ? FALSE : TRUE,
    ) + _tourney_match_contestant($tourney);
  }
  if ($tourney->previous_match2 == 'manual') {
    $user = relation_get_related_entity('tourney_match', $tourney->id, 'contestant_2');
    $form['contestant2'] = array(
      '#title' => t('Contestant 2'),
      '#default_value' => is_object($user) && $user->uid ? $user->uid : -1,
      '#disabled' => is_object($user) && $user->uid ? TRUE : FALSE,
      '#required' => is_object($user) && $user->uid ? FALSE : TRUE,
    ) + _tourney_match_contestant($tourney);
  }
  
  $form['winner'] = array(
    '#type' => 'select',
    '#title' => t('Winner'),
    '#description' => t('The winner of this match.'),
    '#options' => drupal_map_assoc(array('No Winner')),
    '#default_value' => isset($tourney->winner) ? $tourney->winner : -1,
  );

  field_attach_form('tourney_match', $tourney, $form, $form_state);

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
  );

  $form['#submit'][] = 'tourney_match_form_submit';
  

  return $form;
}

/**
 * Create an entity field for contestants.
 */
function _tourney_match_contestant($tourney) {
  return array(
    '#type' => 'select',
    '#options' => _tourney_match_users(),
    '#default_value' => '',
    '#required' => TRUE,
  );
}

/**
 * Return options of users to be used in a select list
 */
function _tourney_match_users() {
  $options = array('' => ' - Choose contestant - ');
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'user')
    ->propertyCondition('uid', 1, '>');
    
  $result = $query->execute();
  if (isset($result['user'])) {
    $uids = array_keys($result['user']);
    $users = entity_load('user', $uids);
    foreach ($users as $uid => $user) {
      $options[$uid] = $user->name;
    }
  }
  return $options;
}

/**
 * Validation function for matches
 */
function tourney_match_form_validate($form, &$form_state) {
  if (isset($form_state['values']['contestant1']) && isset($form_state['values']['contestant2']) && $form_state['values']['contestant1'] == $form_state['values']['contestant2']) {
    form_set_error('contestant1', t('Contestant 1 and Contestant 2 cannot be the same person.'));
  }
}

/**
 * Match form submit handler.
 */
function tourney_match_form_submit($form, &$form_state) {
  drupal_set_message(t('Your match has been saved.'));
  
  $tourney = $form_state['tourney'];
  entity_form_submit_build_entity('tourney_match', $tourney, $form, $form_state);
  tourney_match_save($tourney);
  
  $form_state['redirect'] = 'tourney/match/'. $tourney->id;
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
  drupal_set_title(t('Edit Game: @title', array('@title' => $game->title)));
  
  return drupal_get_form('tourney_game_form', $game);
}

/**
 * The match form.
 */
function tourney_game_form($form, &$form_state, $tourney) {
  $tourney->type = 'game';
  $form_state['tourney'] = $tourney;  
  
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Game Name'),
    '#description' => t('Enter the name of this game.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => isset($tourney->title) ? $tourney->title : '',
  );
  $form['winner'] = array(
    '#type' => 'select',
    '#title' => t('Winner'),
    '#description' => t('The winner of this game.'),
    '#options' => drupal_map_assoc(array('No Winner')),
    '#default_value' => isset($tourney->winner) ? $tourney->winner : -1,
  );

  field_attach_form('tourney_game', $tourney, $form, $form_state);

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
  );

  $form['#submit'][] = 'tourney_game_form_submit';
  

  return $form;
}

/**
 * Game form submit handler.
 */
function tourney_game_form_submit($form, &$form_state) {
  drupal_set_message(t('Your game has been saved.'));
  
  $tourney = $form_state['tourney'];
  entity_form_submit_build_entity('tourney_game', $tourney, $form, $form_state);
  tourney_game_save($tourney);
  
  $form_state['redirect'] = 'tourney/game/'. $tourney->id;
}

/**
 * Implements hook_ENTITY_view().
 */
function tourney_tourney_match_view($entity, $type, $view_mode, $langcode = 'en') {
  $entity->content['#attached']['css'] = array(
    drupal_get_path('module', 'tourney'). '/theme/tourney.css'
  );
  $entity->content['match_winner'] = array(
    '#prefix' => '<div class="field-label">Winner:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $entity->winner,
  );
  $contestant1 = relation_get_related_entity('tourney_match', $entity->id, 'contestant_1');
  $entity->content['match_contestant1'] = array(
    '#prefix' => '<div class="field-label">Contestant 1:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $contestant1->name,
  );
  $contestant2 = relation_get_related_entity('tourney_match', $entity->id, 'contestant_2');
  $entity->content['match_contestant2'] = array(
    '#prefix' => '<div class="field-label">Contestant 2:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $contestant2->name,
  );
}

/**
 * Implements hook_ENTITY_view().
 */
function tourney_tourney_game_view($entity, $type, $view_mode, $langcode = 'en') {
  $entity->content['#attached']['css'] = array(
    drupal_get_path('module', 'tourney'). '/theme/tourney.css'
  );
  $entity->content['match_winner'] = array(
    '#prefix' => '<div class="field-label">Winner:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $entity->winner,
  );
}