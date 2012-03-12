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

function tourney_game_win($match, $game, $contestant) {
  $game->winner = $contestant;
  tourney_game_save($game);
  drupal_goto("/tourney/match/" . $match->id);
}

function tourney_game_win_form_submit($form, &$form_state) {
  $extra = $form_state['values']['extra'];
  $contestant = $form_state['input']['contestant'];
  $match = tourney_match_load($extra['match']);
  $game = tourney_game_load($extra['game']);
  tourney_game_win($match, $game, $contestant);
}

function tourney_game_win_form_js($form, &$form_state) {
  // this is not working at all... .. .
  exit;
  $_POST = array();
  $extra = $form['extra']['#value'];
  $contestant = $form['contestant']['#value'];
  $match = tourney_match_load($extra['match']);
  
  $output = theme('tourney_render_match', array('match' => $match));
  ajax_deliver($output);
  exit;  
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
      '#value' => $extra['name'] . ' wins',
      '#aajax' => array(
        'callback' => 'tourney_game_win_form_js',
        'wrapper' => 'game-list',
        'method' => 'replace',
        'effect' => 'fade',
      ),
    ),
  );
  return $form;
}

/**
 * The match form.
 */
function tourney_match_form($form, &$form_state, $tourney) {
  $tourney->type = 'match';
  $form_state['tourney'] = $tourney;
  
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
  
  $form['games'] = array(
    '#type' => 'textfield',
    '#title' => t('Games'),
    '#description' => t('Enter the number of games in this match'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => $tourney->games,
    '#required' => TRUE,
  );
  
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
    '#options' => _tourney_match_contestants($tourney),
    '#default_value' => '',
    '#required' => TRUE,
  );
}

function _tourney_match_contestants($tourney) {
  $query = db_select('relation_bundles', 'rb');
  $query->fields('rb', array('entity_type'))
    ->distinct()
    ->condition('relation_type', array('contestant_1', 'contestant_2'))
    ->condition('r_index', 1);
  $bundles = $query->execute();
  
  $options = array('' => ' - Choose contestant -');
  foreach ( $bundles as $bundle ) {
    $bn = $bundle->entity_type;
    $group = array();
    $entities = entity_load($bn);
    
    if ( empty($entities) ) {
      $group[] = ' - Empty - ';
    }
    else foreach ( $entities as $eid => $entity ) {
      $title = !property_exists($entity, 'title') ? $entity->name : $entity->title;
      $group["$bn.$eid"] = $title;
    }
    if ( !empty($group) ) $options[$bn] = $group;
  }
  _tourney_match_contestants_sanitize($tourney, $options);
  return $options;
}

function _tourney_match_contestants_sanitize($tourney, &$options) {
  // Get the tournament object this match is in.
  $tournament = relation_get_related_entity('tourney_match', $tourney->id, 'has_match');
  // Get all the matches in round 1 for this tournament
  $query = relation_query('tourney_tournament', $tournament->id);
  $query->entityCondition('bundle', 'has_match')
        ->fieldCondition('round', 'value', 'round-1');
  $results = $query->execute();
  
  $match_ids = array();
  foreach ($results as $relation) {
    $r = relation_load($relation->rid);
    $match_ids[] = $r->endpoints[LANGUAGE_NONE][1]['entity_id'];
  }
  $matches = tourney_match_load_multiple($match_ids);
  
  // Unset users in the options array that are already selected
  foreach ($matches as $match) {
    if (is_object($match->contestant1)) {
      $et = $match->contestant1->_entity_type;
      unset($options[$et]["$et.".$match->contestant1->_entity_id]);
    }
    if (is_object($match->contestant2)) {
      $et = $match->contestant2->_entity_type;
      unset($options[$et]["$et.".$match->contestant2->_entity_id]);
    }
  }
  if ( array_key_exists('user', $options) ) {
    foreach ( $options['user'] as $id => $user ) if ( str_replace("user.", "", $id)  <= 1 ) unset($options['user'][$id]);
  }
}

/**
 * Validation function for matches
 */
function tourney_match_form_validate($form, &$form_state) {
  if (!is_int($form_state['values']['games']) && $form_state['values']['games'] % 2 === 0) {
    form_set_error('games', t('The number of games must be an odd number'));
  }
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
  $form['winner'] = array(
    '#type' => 'select',
    '#title' => t('Winner'),
    '#description' => t('The winner of this game.'),
    '#options' => isset($tourney->contestants) ? array('' => t('- Choose Winner -')) + $tourney->contestants : array(),
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
  $entity->content['match_contestant1'] = is_object($contestant1) ? array(
    '#prefix' => '<div class="field-label">Contestant 1:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $contestant1->name,
  ) : '';
  
  $contestant2 = relation_get_related_entity('tourney_match', $entity->id, 'contestant_2');
  $entity->content['match_contestant2'] = is_object($contestant2) ? array(
    '#prefix' => '<div class="field-label">Contestant 2:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $contestant2->name,
  ) : '';
  
  $entity->content['match_table'] = array(
    '#theme' => 'tourney_render_match',
    '#match' => $entity,
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

