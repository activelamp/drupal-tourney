<?php
/**
* The tournament form.
 */
function tourney_series_form($form, &$form_state, $series) {
  // Set the bundle name
  $series->type = 'series';
  $form_state['series'] = $series;

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Series Name'),
    '#description' => t('Enter the name of this series.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => isset($series->title) ? $series->title : '',
  );

  $form['tournaments_table'] = array(
    '#markup' => tourney_series_tournaments_table($series),
  );
  if(user_access('add tourney to series')) {
  $form['attach_tourneys'] = array(
    '#title' => t('Attach existing tournaments'),
    '#type' => 'textfield',
    '#autocomplete_path' => 'tourney_series/autocomplete',
  );
  $form['attach_add'] = array(
    '#type' => 'button',
    '#value' => 'Add',
    '#validate' => array('tourney_series_autocomplete_form_validate'),
  );
  }

  return $form;
}

function series_name_exists($value) {
  return db_query_range('SELECT 1 FROM {tourney_series} WHERE title = :title', 0, 1, array(':title' => $value))->fetchField();
}


/**
 * Page callback. Edit a series.
 */
function tourney_series_edit($series) {
  drupal_set_title(t('Edit Series: @title', array('@title' => $series->label)));

  return drupal_get_form('tourney_series_form', $series);
}

function tourney_series_tournaments_table($series) {
  $tournaments = tourney_series_get_tournaments_on_series($series);
  //has_tournament
  //'?destination=' . current_path()
  $add_link = l('Add a tournament to this series', 'admin/tourney-series-tournament-add/'. $series->id, array('query' => array('destination' => current_path())));
  $header = array('Tournament Name', 'Tournament Style', 'Players', 'Winner', 'View', 'Edit', 'Delete', 'Remove');

  $rows = array();
  if ( empty($tournaments) ) {
    $message = t('There are currently no tournaments associated with this series.');

    $rows[] = array(
      array('data' => $message, 'colspan' => 3),
    );
  }
  else foreach ($tournaments as $t) {
    // $tournament is not a fully loaded object. Need to get the path via the
    // static method on tournaments.

    $uri = TourneyTournamentEntity::path($t->id);
    $info = TourneyController::getPluginInfo($t->format);
    $rows[] = array(
      l($t->label, $uri),
      $info['name'],
      $t->players,
      $t->winner,
      user_access('access tourney') ? l(t('View'), $uri . '') : '',
      user_access('edit tourney') ? l(t('Edit'), $uri . '/edit', array('query' => array('destination' => current_path()))) : '',
      user_access('delete tourney') ? l(t('Delete'), $uri . '/delete', array('query' => array('destination' => current_path()))) : '',
      user_access('remove tourney from series') ? l(t('Remove'), 'admin/tourney-series-tournament-remove/' . $t->id . '/' . $series->id, array('query' => array('destination' => current_path()))) : '',
    );

  }

  return $add_link . theme('table', array('header' => $header, 'rows' => $rows)) . theme('pager');
}

function tourney_series_remove_tourney($tourney_id, $series_id) {
  $relation = get_tourney_series_relation($tourney_id, $series_id);
}