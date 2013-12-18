<?php
/**
 * @file
 * Pathauto integration.
 */

/**
 * Implements hook_pathauto().
 */
function tourney_league_pathauto($op) {
  switch ($op) {
    case 'settings':
      $pathauto = array();

      $settings = array();
      $entity_type = 'tourney_league';
      $settings['module'] = $entity_type;
      $settings['token_type'] = $entity_type;
      $settings['groupheader'] = 'Tourney League ' . t('paths');
      $settings['patterndescr'] = t('Default path pattern (applies to all content types with blank patterns below)');
      $settings['patterndefault'] = '';
      $settings['batch_update_callback'] = 'tourney_league_pathauto_bulk_update_batch_process';
      $settings['batch_file'] = drupal_get_path('module', 'tourney_league') . '/tourney_league.pathauto.inc';

      $pathauto[$entity_type] = (object) $settings;
      return $pathauto;

    default:
      break;
  }
}

/**
 * Batch processing callback; Generate aliases for entities.
 */
function tourney_league_pathauto_bulk_update_batch_process(&$context) {
  $entity_type = 'tourney_league';
  if (!isset($context['sandbox']['count'])) {
    // First pass. Build an array with all the pairs of id and entity type to process.
    $context['sandbox']['count'] = 0;
    $context['sandbox']['items'] = array();

    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', $entity_type);
    $result = $query->execute();
    $ids = isset($result[$entity_type]) ? array_keys($result[$entity_type]) : array();
    foreach ($ids as $id) {
      $context['sandbox']['items'][] = array('id' => $id, 'type' => $entity_type);
    }
    $context['sandbox']['total'] = count($context['sandbox']['items']);

    // If there are no items to update, stop immediately.
    if (!$context['sandbox']['total']) {
      $context['finished'] = 1;
      return;
    }
  }

  // Extract 25 items to be processed in this pass.
  $items_to_process = array_slice($context['sandbox']['items'], $context['sandbox']['count'], 25);
  module_load_include('inc', 'pathauto');
  foreach ($items_to_process as $item) {
    $entity = entity_load($item['type'], array($item['id']));
    $entity = reset($entity);
    tourney_update_alias($entity, 'bulkupdate');
  }

  // Update progress stats.
  $context['sandbox']['count'] += count($items_to_process);
  if ($context['sandbox']['count'] != $context['sandbox']['total']) {
    $context['finished'] = $context['sandbox']['count'] / $context['sandbox']['total'];
  }
}
