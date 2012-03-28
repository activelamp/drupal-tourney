<?php

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
      'click sortable' => TRUE,
    ),
    'argument' => array(
      'real field' => 'id',
      'handler' => 'tourney_handler_match_tree',
    ),
  );
}

function tourney_views_plugins() {
  $path = drupal_get_path('module', 'tourney');
  $plugins = array();
  $plugins['style'] = array(
    'tournament' => array(
      'title' => t('Tournament Tree'),
      'help' => t('Displays a bracket tree'),
      'handler' => 'tourney_plugin_style_tournament',
      'theme' => 'tourney_view_tournament',
      'theme path' => $path . '/theme',
      'theme file' => 'tourney-view-tournament.tpl.php',
      'uses row plugin' => TRUE,
      'uses row class' => TRUE,
      'uses grouping' => TRUE,
      'uses options' => TRUE,
      'type' => 'normal',
    ),
  );
  return $plugins;
}