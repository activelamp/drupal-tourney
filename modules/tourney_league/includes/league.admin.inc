<?php

/**
 * @file
 * Admin callbacks for league entities..
 */

/**
 * Page callback.
 */
function tourney_league_admin_content() {  
  // load all the leagues.
  $leagues = entity_load('tourney_league');

  // table header
  $header = array("ID", "Label", "Edit", "Delete");

  // table rows
  $rows = array();
  foreach($leagues as $id => $league) {
    $row = array();
    $row[] = $id;
    $row[] = l($league->label, "tourney/league/$id");
    $row[] = l("Edit", "tourney/league/$id/edit");
    $row[] = l("Delete", "tourney/league/$id/delete");
    $rows[] = $row;
  }

  return theme_table(
    array(
      "header" => $header, 
      "rows" => $rows, 
      "attributes" => array(),
      "caption" => "",
      "colgroups" => array(),
      "sticky" => array(),
      "empty" => array()
    )
  );
}

/**
 * Returns content that drupal can use to render a page
 */
function tourney_league_view_page($league, $view_mode = 'full') {
  $league->content = array();
  field_attach_prepare_view('league', array($league->id => $league), $view_mode);
  entity_prepare_view('league', array($league->id => $league));
  $league->content += field_attach_view('league', $league, $view_mode);
  return $league->content;
}

/**
 * Returns a custom title
 */
function tourney_league_page_title($league) {
  return "$league->entity_name ($league->id)";
}

/**
 * Delete page callback
 */
function tourney_league_confirm_delete_page($form, &$form_state, $league) {
  $form = array();

  $form["#league"] = $league;
  return confirm_form(
    $form,
    t('Are you sure you want to delete the league %title?', array('%title' => $league->label)),
    "tourney/league/$league->id",
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel'),
    'tourney_league_confirm_delete_page_submit'
  );
}


/**
 * Returns the information of a bundle for the league entity
 */
function tourney_league_types_information($league_type) {
  return $league_type->type . ': ' . $league_type->description;
}

/**
 * Form defintion for the league forms
 */
function tourney_league_form($form, &$form_state, $league) {
  $form['#id'] = 'league-form';

  // store the league for later
  $form['#league'] = $league;
  $form_state['league'] = $league;

  // label field
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('League Name'),
    '#default_value' => isset($league->label) ? $league->label : '',
    '#weight' => -5,
    '#required' => TRUE,
  );

  // buttons
  $form['buttons'] = array();
  $form['buttons']['#weight'] = 100;
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('tourney_league_form_submit'),
  );

 if (!empty($league->id)) {
   $form['buttons']['delete'] = array(
     '#access' => user_access('delete league'),
     '#type' => 'submit',
     '#value' => t('Delete'),
     '#weight' => 15,
     '#submit' => array('league_form_delete_submit'),
   );
 }

  $form['#validate'][] = 'tourney_league_form_validate';
  field_attach_form('tourney_league', $league, $form, $form_state);

  return $form;
}

/**
 * Validate hook for league forms
 */
function tourney_league_form_validate($form, &$form_state) {
  $league = $form_state['league'];
  field_attach_form_validate('tourney_league', $league, $form, $form_state);
}

/**
 * Implements league_form_submit
 */
function tourney_league_form_submit($form, &$form_state) {
  $league = &$form_state['league'];

  entity_form_submit_build_entity('tourney_league', $league, $form, $form_state);
  $league->save();
  $form_state['redirect'] = 'tourney/league/' . $league->id;
}

/**
 * Implements league_form_delete_submit
 */
function league_form_delete_submit($form, &$form_state) {
  tourney_league_confirm_delete_page_submit($form, $form_state);
}

/**
 * Show the tournament page.
 */
function tourney_league_page_view($tourney, $view_mode = 'full') {
  return $tourney->view($mode, NULL, TRUE);
}

/**
 * Page callback. Edit a tournament.
 */
function tourney_league_edit($league) {
  drupal_set_title(t('Edit League: @title', array('@title' => $league->label)));
  return drupal_get_form('tourney_league_form', $league);
}

/**
 * Delete submit callback.
 */
function tourney_league_confirm_delete_page_submit($form, &$form_state) {
  entity_delete('tourney_league', $form['#league']->id);
  $form_state['redirect'] = "admin/content/league";
}
