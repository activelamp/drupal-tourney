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
  if  (TOURNEY_DEBUG_EXTENDS) {
    $match->isStarted();
    $match->determineWinner();
  }
  return $match->view('full', NULL, TRUE);
}

/**
 * Page callback. Edit a match.
 */
function tourney_match_edit($match) {
  drupal_set_title(t('Edit Match: @title', array('@title' => $match->title)));

  return drupal_get_form('tourney_match_form', $match);
}

function tourney_game_win_form_submit($form, &$form_state) {
  $extra = $form_state['values']['extra'];
  $contestant = $form_state['input']['contestant'];
  $match = tourney_match_load($extra['match']);
  $game = tourney_game_load($extra['game']);
  $game->winner = $contestant;
  tourney_game_save($game);
  $form_sate['redirect'] = 'tourney/match/' . $match->id;
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
function tourney_match_form($form, &$form_state, $match) {
  $match->type = 'match';
  $form_state['tourney'] = $match;
  $games = tourney_tournament_get_match_games($match);
  $match_started = $match->isStarted();
  $tournament_finished = $match->tournament->isFinished();

  // @todo: It would be nice if these were field api fields
  $contestant = relation_get_related_entity('tourney_match', $match->id, TOURNEY_CONTESTANT_1);
  $c_1_uid = (is_object($contestant) && ($contestant->uid > 0)) ? $contestant->uid : -1;
  $form['contestant1'] = array(
    '#title' => t('Contestant 1'),
    '#default_value' => $c_1_uid,
    '#disabled' => ($match_finished) ? TRUE : FALSE,
    '#required' => ($match_finished) ? FALSE : TRUE,
  ) + _tourney_match_contestant($match);

  $contestant = relation_get_related_entity('tourney_match', $match->id, TOURNEY_CONTESTANT_2);
  if (($match->previous_match2 == 'manual' && !is_object($contestant))) {
    $form['contestant2'] = array(
      '#title' => t('Contestant 2'),
      '#default_value' => is_object($contestant) && $contestant->uid ? $contestant->uid : -1,
      '#disabled' => is_object($contestant) && $contestant->uid ? TRUE : FALSE,
      '#required' => is_object($contestant) && $contestant->uid ? FALSE : TRUE,
    ) + _tourney_match_contestant($match);
  }

  $form['games'] = array(
    '#type' => 'textfield',
    '#title' => t('Games'),
    '#description' => t('Enter the number of games in this match'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => $match->games,
    '#required' => TRUE,
    '#disabled' => (count($games) > 1) || $tournament_finished,
  );

  field_attach_form('tourney_match', $match, $form, $form_state);

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
    '#disabled' => $tournament_finished
  );

  $form['#submit'][] = 'tourney_match_form_submit';

  return $form;
}

/**
 * Create an entity field for contestants.
 */
function _tourney_match_contestant($match) {
  return array(
    '#type' => 'select',
    '#options' => _tourney_match_contestants($match),
    '#default_value' => '',
    '#required' => TRUE,
  );
}

function _tourney_match_contestants($match) {
  $query = db_select('relation_bundles', 'rb');
  $query->fields('rb', array('entity_type'))
    ->distinct()
    ->condition('relation_type', array(TOURNEY_CONTESTANT_1, TOURNEY_CONTESTANT_2))
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
  _tourney_match_contestants_sanitize($match, $options);
  return $options;
}

function _tourney_match_contestants_sanitize($main_match, &$options) {
  // Get the tournament object this match is in.
  $tournament = relation_get_related_entity('tourney_match', $main_match->id, 'has_match');
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
      unset($options[$et]["$et." . $match->contestant1->_entity_id]);
    }
    if (is_object($match->contestant2)) {
      $et = $match->contestant2->_entity_type;
      unset($options[$et]["$et." . $match->contestant2->_entity_id]);
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
  $match = $form_state['tourney'];

  if (!$match->tournament->isFinished()) {
    drupal_set_message(t('Your match has been saved.'));
    entity_form_submit_build_entity('tourney_match', $match, $form, $form_state);
    tourney_match_save($match);
  }
  else {
    drupal_set_message(t('Tournament is already finished. Match not saved.'), 'error');
  }

  $form_state['redirect'] = 'tourney/match/' . $match->id;
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

  $form_state['redirect'] = 'tourney/game/' . $tourney->id;
}

/**
 * Implements hook_ENTITY_view().
 */
function tourney_tourney_match_view($entity, $type, $view_mode, $langcode = 'en') {
  $entity->content['#attached']['css'] = array(
    drupal_get_path('module', 'tourney') . '/theme/tourney.css'
  );
  $entity->content['#attached']['js'] = array(
    drupal_get_path('module', 'tourney') . '/theme/tourney.js'
  );
  $entity->content['tournament'] = array(
    '#prefix' => '<div class="field-label">Tournament:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#theme' => 'link',
    '#text' => 'Go to tournament ' . $entity->tournament->title,
    '#path' => 'tourney/' . $entity->tournament->id,
    '#options' => array('attributes' => array(), 'html' => FALSE),
  );
  $entity->content['match_winner'] = array(
    '#prefix' => '<div class="field-label">Winner:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $entity->winner,
  );
  $contestant1 = relation_get_related_entity('tourney_match', $entity->id, TOURNEY_CONTESTANT_1);
  $entity->content['match_contestant1'] = is_object($contestant1) ? array(
    '#prefix' => '<div class="field-label">Contestant 1:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $contestant1->name,
  ) : '';

  $contestant2 = relation_get_related_entity('tourney_match', $entity->id, TOURNEY_CONTESTANT_2);
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
    drupal_get_path('module', 'tourney') . '/theme/tourney.css'
  );
  $entity->content['#attached']['js'] = array(
    drupal_get_path('module', 'tourney') . '/theme/tourney.js'
  );
  $entity->content['match_winner'] = array(
    '#prefix' => '<div class="field-label">Winner:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $entity->winner,
  );
}

function tourney_match_get_parents($mid, $depth) {
  $ids = array($mid);

  $match = tourney_match_load($mid);
  $tournament = $match->tournament;
  $matches = tourney_tournament_build_matches($tournament);
  $bracket = tourney_get_name_part($match->this_match, 'bracket');

  $names = array($match->previous_match1, $match->previous_match2);
  while ( $depth-- > 0 ) {
    $new_names = array();
    foreach ( $names as $name ) {
      $nmatch = tourney_match_get_by_name($matches, $name);
      if ( !$nmatch ) continue;
      if ( tourney_get_name_part($nmatch->this_match, 'bracket') != $bracket ) continue;
      $ids[] = $nmatch->id;
      $new_names = array_merge($new_names, array($nmatch->previous_match1, $nmatch->previous_match2));
    }
    $names = $new_names;
  }

  return $ids;
}

function tourney_match_get_by_name($matches, $name) {
  if ( $name == 'manual' ) return NULL;
  $parts = explode('_', $name);
  foreach ( $parts as $part ) if ( array_key_exists($part, $matches) ) $matches = $matches[$part];
  @list(, $id) = explode('-', array_pop($parts));
  while ( --$id > 0 ) array_shift($matches);
  $match = array_shift($matches);
  return $match['match'];
}

function tourney_get_name_part($name, $part) {
  foreach ( explode('_', $name) as $sub ) {
    $sub = explode('-', $sub);
    if ( $sub[0] == $part ) return $sub[1];
  }
}
