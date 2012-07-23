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

  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($series->entity_name) ? $series->entity_name : '',
    '#maxlength' => 21,
    '#description' => t('A unique name for a series. It must only contain lowercase letters, numbers and hyphens.'),
    '#machine_name' => array(
      'source' => array('label'),
      'exists' => 'series_name_exists',
      'replace_pattern' => '[^a-z0-9-]+',
      'replace' => '-',
    ),
  );

  $path = array();
  if ( !empty($series->entity_id) ) {
    $conditions = array('source' => 'tourney/series/' . $series->entity_id);
    $path = path_load($conditions);
    if ( $path === FALSE ) {
      $path = array();
    }
  }

  $form['path'] = array(
    '#type' => 'fieldset',
    '#title' => t('URL path settings'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($path['alias']),
    '#group' => 'additional_settings',
    '#attributes' => array(
      'class' => array('path-form'),
    ),
    '#attached' => array(
      'js' => array(drupal_get_path('module', 'path') . '/path.js'),
    ),
    '#access' => user_access('create url aliases') || user_access('administer url aliases'),
    '#weight' => 30,
    '#tree' => TRUE,
    '#element_validate' => array('path_form_element_validate'),
  );

  $path += array(
    'pid' => NULL,
    'source' => isset($series->entity_id) ? 'tourney/series/' . $series->entity_id : NULL,
    'alias' => '',
    'language' => isset($series->language) ? $series->language : LANGUAGE_NONE,
  );

  $form['path']['alias'] = array(
    '#type' => 'textfield',
    '#title' => t('URL alias'),
    '#default_value' => $path['alias'],
    '#maxlength' => 255,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('Optionally specify an alternative URL by which this content can be accessed. For example, type "about" when writing an about page. Use a relative path and don\'t add a trailing slash or the URL alias won\'t work.'),
  );
  $form['path']['pid'] = array(
    '#type' => 'value',
    '#value' => $path['pid'],
  );
  $form['path']['source'] = array(
    '#type' => 'value',
    '#value' => $path['source'],
  );
  $form['path']['language'] = array(
    '#type' => 'value',
    '#value' => $path['language'],
  );

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Publishing Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#weight' => 95,
  );

  $form['options']['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Published'),
    '#default_value' => isset($series->status) ? $series->status : 0,
  );

  field_attach_form('tourney_series', $series, $form, $form_state);
  $form['tournaments_table'] = array(
    '#markup' => tourney_series_tournaments_table($series),
  );
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
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 10,
  );

  if (!empty($series->id)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 40,
      '#submit' => array('tourney_series_delete_submit'),
    );
  }

  $form['#submit'][] = 'tourney_series_form_submit';

  return $form;
}

function series_name_exists($value) {
  return db_query_range('SELECT 1 FROM {tourney_series} WHERE name = :name', 0, 1, array(':name' => $value))->fetchField();
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
  $header = array('Tournament Name', 'Tournament Style', 'Players', 'Winner', 'View', 'Edit', 'Delete');

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
    );

  }

  return $add_link . theme('table', array('header' => $header, 'rows' => $rows)) . theme('pager');
}