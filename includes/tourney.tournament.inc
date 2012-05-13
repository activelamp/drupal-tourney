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
  
  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($tourney->title) ? $tourney->name : '',
    '#maxlength' => 21,
    '#description' => t('A unique name for tournaments. It must only contain lowercase letters, numbers and hyphens.'),
    '#machine_name' => array(
      'source' => array('title'),
      'exists' => 'tourney_name_exists',
      'replace_pattern' => '[^a-z0-9-]+',
      'replace' => '-',
    ),
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
 * Tournament form submit handler.
 */
function tourney_tournament_form_submit($form, &$form_state) {
  $tourney = $form_state['tourney'];
  entity_form_submit_build_entity('tourney_tournament', $tourney, $form, $form_state);
  tourney_tournament_save($tourney);
  
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
   '#markup' => $info['name'],
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
    '#theme' => 'tourney_render_tourney',
    '#tournament' => $entity,
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
  $cache_name = 'tourney_match_games_' . $entity->id . ( $load_entity ? '_entity' : '' );
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
  drupal_set_title(t('Edit Tournament: @title', array('@title' => $tournament->title)));

  return drupal_get_form('tourney_tournament_form', $tournament);
}
