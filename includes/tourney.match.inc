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
function tourney_match_page_view($tourney) {
  return $tourney->view('full', 'en', 'page');
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
    '#title' => t('Tournament Name'),
    '#description' => t('Enter the name of this tournament.'),
    '#size' => 40,
    '#maxlength' => 255,
    '#default_value' => isset($tourney->title) ? $tourney->title : '',
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
 * Match form submit handler.
 */
function tourney_match_form_submit($form, &$form_state) {
  drupal_set_message(t('Your match has been saved.'));
  
  $tourney = $form_state['tourney'];
  entity_form_submit_build_entity('tourney_match', $tourney, $form, $form_state);
  tourney_match_save($tourney);
}

/**
 * Show the tournament page.
 */
function tourney_game_page_view($tourney) {
  return $tourney->view('full', 'en', 'page');
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
}