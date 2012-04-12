<?php

/*
 * @file
 */

/**
 * Implements hook_views_data_alter().
 */
function tourney_views_data_alter(&$data) {
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