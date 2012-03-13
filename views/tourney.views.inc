<?php

/**
 * Implements hook_views_data_alter().
 */
function tourney_views_data_alter(&$data) {
  $data['tourney_match']['match_with_parents'] = array(
    'title' => t('Match with Parents'),
    'help' => t('Match with the number of generations to go up'),
    'field' => array(
      'real field' => 'title',
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'real field' => 'title',
      'handler' => 'tourney_handler_match_with_parents',
    ),
  );
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

