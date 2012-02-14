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
  
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Tournament Name'),
    '#description' => t('Enter the name of this tournament.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => isset($tourney->title) ? $tourney->title : '',
  );
  
  $form['format'] = array(
    '#type' => 'select',
    '#title' => t('Tournament style'),
    '#description' => t('Choose the style of play for this tournament'),
    '#options' => tourney_get_type_options(),
    '#disabled' => isset($tourney->format) ? TRUE : FALSE,
    '#default_value' => isset($tourney->format) ? $tourney->format : -1,
  );
  
  $form['players'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of contestants'),
    '#description' => t('Enter the number contestants playing in this tournament'),
    '#size' => 10,
    '#maxlength' => 2,
    '#disabled' => isset($tourney->players) ? TRUE : FALSE,
    '#default_value' => isset($tourney->players) ? $tourney->players : '',
  );
  
  $form['winner'] = array(
    '#type' => 'select',
    '#title' => t('Winner'),
    '#description' => t('The winner of this tournament.'),
    '#options' => drupal_map_assoc(array('No Winner')),
    '#default_value' => isset($tourney->winner) ? $tourney->winner : -1,
  );
  
  field_attach_form('tourney_tournament', $tourney, $form, $form_state);
  
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
  
  $tourney = $form_state['tourney'];
  entity_form_submit_build_entity('tourney_tournament', $tourney, $form, $form_state);
  tourney_tournament_save($tourney);
  
  $form_state['redirect'] = 'tourney/'. $tourney->id;
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
  $entity->content['#attached']['css'] = array(
    drupal_get_path('module', 'tourney'). '/theme/tourney.css'
  );
  $entity->content['tourney_format'] = array(
   '#prefix' => '<div class="field-label">Tournament Format:</div><div class="field-content">',
   '#suffix' => '</div>',
   '#markup' => $entity->format,
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
    '#theme' => 'tourney_render_matches',
    '#matches' => _tourney_tournament_build_matches($entity),
  );
}

function _tourney_tournament_build_matches($entity) {
  $query = relation_query('tourney_tournament', $entity->id);
  $query->entityCondition('bundle', 'has_match');
  $results = $query->execute();

  $matches = array();
  foreach ($results as $id => $relation) {
    $relation = relation_load($id);
    $bracket = field_get_items('relation', $relation, 'bracket');
    $round = field_get_items('relation', $relation, 'round');
    $endpoints = field_get_items('relation', $relation, 'endpoints');

    $match = tourney_match_load($endpoints[1]['entity_id']);    
    if ($bracket[0]['safe_value'] && $round[0]['safe_value']) {
      $matches[$bracket[0]['safe_value']][$round[0]['safe_value']][$match->id] = array(
        'match' => $match,
        'games' => _tourney_tournament_build_games($match),
      );
    }
    else if ($round[0]['safe_value']) {
      $matches[$round[0]['safe_value']][$match->id] = array(
        'match' => $match,
        'games' => _tourney_tournament_build_games($match),
      );
    }
    else {
      $matches[$match->id] = array(
        'match' => $match,
        'games' => _tourney_tournament_build_games($match),
      );
    }
  }
  return $matches;
}

function _tourney_tournament_build_games($entity) {
  $query = relation_query('tourney_match', $entity->id);
  $query->entityCondition('bundle', 'has_game');
  $results = $query->execute();

  $games = array();
  foreach ($results as $id => $relation) {
    $relation = relation_load($id);
    $endpoints = field_get_items('relation', $relation, 'endpoints');

    $game = tourney_game_load($endpoints[1]['entity_id']);    
    $games[$game->id] = $game;
  }
  return $games;
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
    $items[] = l($label, 'admin/structure/tourney/'. $entity_short);
  }
  return theme('item_list', array('items' => $items));
}

/**
 * Page callback. Edit a tournament.
 */
function tourney_tournament_edit($tournament) {
  drupal_set_title(t('Edit Tournament: @title', array('@title' => $tournament->title)));
  
  return drupal_get_form('tourney_tournament_form', $tournament);
}