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
  $form_state['redirect'] = 'admin/structure/tournament/manage/' . $form_state['tourney_tournament_type']->type . '/delete';
}

/**
 * Page callback.
 */
function tourney_admin_content() {
  $output = '';
  $header = array(
    'id' => array('data' => t('Id'), 'field' => 't.id', 'sort' => 'asc'),
    'title' => array('data' => t('Title'), 'field' => 't.title'),
    'format' => array('data' => t('Format'), 'field' => 't.format'),
    'players' => array('data' => t('Players'), 'field' => 't.players'),
    'winner' => array('data' => t('Winner'), 'field' => 't.winner'),
    'operations' => array('data' => t('Operations'), 'colspan' => '3'),
  );

  // Grab all relations.
  $query = db_select('tourney_tournament', 't')->extend('PagerDefault')->extend('TableSort');
  $query->fields('t', array('id', 'title', 'format', 'players', 'winner'))
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
  $tournaments = $variables['tournaments'];
  $header = $variables['header'];

  $rows = array();
  if ( empty($tournaments) ) {
    $message = t('There are currently no tournaments on your site.');

    $rows[] = array(
      array('data' => $message, 'colspan' => 8),
    );
  }
  else foreach ( $tournaments as $tournament ) {
    $rows[] = array(
      l($tournament->id, 'tourney/' . $tournament->id),
      l($tournament->title, 'tourney/' . $tournament->id),
      $tournament->format,
      $tournament->players,
      $tournament->winner,
      user_access('access content') ? l(t('View'), 'tourney/' . $tournament->id . '') : '',
      user_access('access content') ? l(t('Edit'), 'tourney/' . $tournament->id . '/edit') : '',
      //TODO : implement delete
      user_access('access content') ? l(t('Delete'), 'tourney/' . $tournament->id . '/delete') : '',
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
    t('Are you sure you want to delete the tournament %title?', array('%title' => $tournament->title)),
    'tourney/' . $tournament->id,
    t('All matches belonging to this tournament will be deleted. This action cannot be undone.'),
    t('Delete'),
    t('Cancel'),
    'tourney_confirm_delete');
}

function tourney_confirm_delete_submit($form, &$form_state) {
  $tournament = $form['#tournament'];
  $deletes = array('tourney_tournament.' . $tournament->id);
  $mquery = relation_query('tourney_tournament', $tournament->id)->entityCondition('bundle', 'has_match');
  $matches = $mquery->execute();
  foreach ( $matches as $mrid => $mrv ) {
    $mrelation = relation_load($mrid);
    $match = tourney_match_load($mrelation->endpoints['und'][1]['entity_id']);
    $deletes[] = 'relation.' . $mrid;
    $deletes[] = 'tourney_match.' . $match->id;
    $crids = array();
    $c1q = relation_query('tourney_match', $match->id)->entityCondition('bundle', 'contestant_1');
    $crids += $c1q->execute();
    $c2q = relation_query('tourney_match', $match->id)->entityCondition('bundle', 'contestant_2');
    $crids += $c2q->execute();
    foreach ( $crids as $crid => $crv ) $deletes[] = 'relation.' . $crid;
    $gquery = relation_query('tourney_match', $match->id)->entityCondition('bundle', 'has_game');
    $games = $gquery->execute();
    foreach ( $games as $grid => $grv ) {
      $grelation = relation_load($grid);
      $game = tourney_game_load($grelation->endpoints['und'][1]['entity_id']);
      $deletes[] = 'relation.' . $grid;
      $deletes[] = 'tourney_game.' . $game->id;
    }
  }
  foreach ( $deletes as $delete ) {
    @list($type, $id) = tourney_cid_split($delete);
    if ( $type == 'relation' ) relation_delete($id);
    else entity_delete($type, $id);
    watchdog('content', 'Deleted @type #@id.', array('@type' => $type, '@id' => $id));
  }
  drupal_set_message(t('The tournament and all its matches and games have been deleted.'));
  $form_state['redirect'] = "admin/content/tourney";
}












