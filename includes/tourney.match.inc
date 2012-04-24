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
    ),
  );
  return $form;
}

/**
 * The match form.
 */
function tourney_match_form($form, &$form_state, $match) {
  $match->type = 'match';
  $form_state['match'] = $match;
  $games = tourney_tournament_get_match_games($match);

  // @todo: It would be nice if these were field api fields
  $contestant = relation_get_related_entity('tourney_match', $match->id, TOURNEY_CONTESTANT_1);
  if (($match->previous_match1 == 'manual' && !is_object($contestant))) {
    $form['contestant1'] = array(
      '#title' => t('Contestant 1'),
      '#default_value' => is_object($contestant) && $contestant->uid ? $contestant->uid : -1,
      '#disabled' => is_object($contestant) && $contestant->uid ? TRUE : FALSE,
      '#required' => is_object($contestant) && $contestant->uid ? FALSE : TRUE,
    ) + _tourney_match_contestant($match);
  }

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
    '#disabled' => $m_finished,
  );

  $matches = tourney_tournament_build_matches($match->tournament);
  $next_matches = array();
  if ( $match->next_match_winner )
    $next_matches[] = tourney_get_match_by_name($matches, $match->next_match_winner);
  if ( $match->next_match_loser )
    $next_matches[] = tourney_get_match_by_name($matches, $match->next_match_loser);

  $locked = FALSE;
  foreach ( $next_matches as $next_match )
    if ( count($next_match->games_played) > 1 ) $locked = TRUE;

  if ( $locked ) {
    $form['fix_games'] = array(
      '#markup' => "Sorry, one or more of the next matches have already started, you cannot edit this match's games anymore.",
    );
  }
  elseif ( count($match->games_played) > 1 ) {
    $form['fix_games'] = array(
      '#theme' => 'tourney_match_fix',
      '#games' => $match->games,
    );

    foreach ( $games as $id => $game ) {
      $form['fix_games']['game.' . $id] = array(
        '#type' => 'radios',
        '#title' => t('Game @id', array('@id' => $id)),
        '#default_value' => $game->winner,
        '#options' => $game->contestants,
      );
    }
  }

  field_attach_form('tourney_match', $match, $form, $form_state);

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
    '#disabled' => $m_finished
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
  $query->fields('rb', array('entity_type', 'bundle'))
    ->distinct()
    ->condition('relation_type', array(TOURNEY_CONTESTANT_1, TOURNEY_CONTESTANT_2))
    ->condition('r_index', 1);
  $results = $query->execute()->fetchAll();

  $options = array('' => ' - Choose contestant -');
  foreach ($results as $row) {
    $entity_type = $row->entity_type;
    $group = array();

    if ($row->bundle === '*') {
      $entities = entity_load($entity_type);
    }
    else {
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', $entity_type, '=')
        ->propertyCondition('type', $row->bundle, '=');
      $result = $query->execute();

      $entities = array();
      if (isset($result[$entity_type])) {
        $ids = array_keys($result[$entity_type]);
        $entities = entity_load($entity_type, $ids);
      }
    }

    if (empty($entities)) {
      $group[] = ' - Empty - ';
    }
    else {
      foreach ($entities as $eid => $entity) {
        $title = !property_exists($entity, 'title') ? $entity->name : $entity->title;
        $group["$entity_type.$eid"] = $title;
      }
    }
    if (!empty($group)) {
      $group_name = $row->bundle !== '*' ? $entity_type . ' - ' . $row->bundle : $entity_type;
      $options[$group_name] = $group;
    }
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
  drupal_set_message(t('Your match has been saved.'));

  foreach ( $form_state['input'] as $id => $value ) {
    if ( !$value || substr($id, 0, 5) != 'game_' ) continue;
    $parts = explode('_', $id);
    $game = tourney_game_load($parts[1]);
    if ( $game->winner != $value ) {
      $game->winner = $value;
      tourney_game_save($game);
    }
  }

  $match = $form_state['match'];
  entity_form_submit_build_entity('tourney_match', $match, $form, $form_state);
  tourney_match_save($match);
  if (function_exists('_tourney_set_match_winner')) {
    _tourney_set_match_winner($match);
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
function tourney_game_form($form, &$form_state, $game) {
  $game->type = 'game';
  $form_state['game'] = $game;
  $form['winner'] = array(
    '#type' => 'select',
    '#title' => t('Winner'),
    '#description' => t('The winner of this game.'),
    '#options' => isset($game->contestants) ? array('' => t('- Choose Winner -')) + $game->contestants : array(),
    '#default_value' => isset($game->winner) ? $game->winner : -1,
  );

  field_attach_form('tourney_game', $game, $form, $form_state);

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

  $game = $form_state['game'];
  entity_form_submit_build_entity('tourney_game', $game, $form, $form_state);
  tourney_game_save($game);

  $form_state['redirect'] = 'tourney/game/' . $game->id;
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
    '#markup' => $contestant1->title,
  ) : '';

  $contestant2 = relation_get_related_entity('tourney_match', $entity->id, TOURNEY_CONTESTANT_2);
  $entity->content['match_contestant2'] = is_object($contestant2) ? array(
    '#prefix' => '<div class="field-label">Contestant 2:</div><div class="field-content">',
    '#suffix' => '</div>',
    '#markup' => $contestant2->title,
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
      $nmatch = tourney_get_match_by_name($matches, $name);
      if ( !$nmatch ) continue;
      if ( tourney_get_name_part($nmatch->this_match, 'bracket') != $bracket ) continue;
      $ids[] = $nmatch->id;
      $new_names = array_merge($new_names, array($nmatch->previous_match1, $nmatch->previous_match2));
    }
    $names = $new_names;
  }

  return $ids;
}

function tourney_get_match_by_name($matches, $name) {
  if ( $name == 'manual' ) return NULL;
  $parts = explode('_', $name);
  if ( substr(end($parts), 0, 10) == 'contestant' ) array_pop($parts);
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
