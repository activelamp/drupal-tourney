<?php

/*
 * @file
 */

/**
 * Implements hook_views_data_alter().
 */
function tourney_views_data_alter(&$data) {
  // Tournery Match additions
  $data['tourney_match']['match_tree'] = array(
    'title' => t('Match Tree'),
    'help' => t('Match ID with depth'),
    'field' => array(
      'real field' => 'id',
      'handler' => 'views_handler_field',
    ),
    'argument' => array(
      'real field' => 'id',
      'handler' => 'tourney_handler_match_tree',
    ),
  );
  $data['tourney_match']['match_tree_depth'] = array(
    'title' => t('Match Tree Depth'),
    'help' => t('Match ID custom depth filter'),
    'field' => array(
      'real field' => 'id',
      'handler' => 'views_handler_field',
    ),
    'argument' => array(
      'real field' => 'id',
      'handler' => 'views_handler_argument',
    ),
  );
  // Define the field for the gameIds on the match object
  $data['tourney_match']['game_ids'] = array(
    'title' => t('Game Ids'),
    'help' => t('An array of game ids loaded on the match object'),
    'field' => array(
      'handler' => 'tourney_handler_field_object_property',
    ),
  );
  $data['tourney_match']['tournament_id'] = array(
    'title' => t('Tournament Id'),
    'help' => t('The tournament id loaded on the match object'),
    'field' => array(
      'handler' => 'tourney_handler_field_object_property',
    ),
  );
  
  // Tourney Game additions
  $data['tourney_game']['edit_game'] = array(
    'field' => array(
      'title' => t('Edit link'),
      'help' => t('Provide a simple link to edit the game.'),
      'handler' => 'tourney_handler_field_entity_link_edit',
    ),
  );
  
}

function tourney_views_plugins() {
  $path = drupal_get_path('module', 'tourney');
  return array(
    'module' => 'tourney',
    'style' => array(
      'tourney' => array(
        'title' => 'Tournament Tree',
        'handler' => 'tourney_plugin_style_tournament',
        'path' => "$path/includes",
        'theme' => 'tourney_tournament_view',
        'theme path' => "$path/theme",
        'theme file' => 'tourney_tournament_view.inc',
        'uses fields' => FALSE,
        'uses row plugin' => FALSE,
        'type' => 'normal',
        'even empty' => TRUE,
      ),
    ),
  );
}