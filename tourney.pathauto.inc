<?php
/**
 * @file
 * Pathauto integration.
 */

/**
 * Implements hook_pathauto().
 */
function tourney_pathauto($op) {
  switch ($op) {
    case 'settings':
      // Get entities provied by this module.
      $entities = tourney_entities();

      $pathauto = array();
      foreach ($entities as $entity_type => $entity) {
        $settings = array();
        $settings['module'] = $entity_type;
        $settings['token_type'] = $entity_type;
        $settings['groupheader'] = 'Tourney ' . $entity->name . ' ' . t('paths');
        $settings['patterndescr'] = t('Default path pattern (applies to all content types with blank patterns below)');
        $settings['patterndefault'] = '';
        $settings['batch_update_callback'] = $entity_type . '_pathauto_bulk_update_batch_process';
        $settings['batch_file'] = drupal_get_path('module', 'tourney') . '/tourney.pathauto.inc';

        $languages = array();
        /* Enable when localized enabled.
        if (module_exists('locale')) {
          $languages = array(LANGUAGE_NONE => t('language neutral')) + locale_language_list('name');
        }
        */

        /* @todo: need to figure this out for all tourney entity types bundles.
        foreach (node_type_get_names() as $node_type => $node_name) {
          if (count($languages) && variable_get('language_content_type_' . $node_type, 0)) {
            $settings['patternitems'][$node_type] = t('Default path pattern for @node_type (applies to all @node_type content types with blank patterns below)', array('@node_type' => $node_name));
            foreach ($languages as $lang_code => $lang_name) {
              $settings['patternitems'][$node_type . '_' . $lang_code] = t('Pattern for all @language @node_type paths', array('@node_type' => $node_name, '@language' => $lang_name));
            }
          }
          else {
            $settings['patternitems'][$node_type] = t('Pattern for all @node_type paths', array('@node_type' => $node_name));
          }
        }
        */

        $pathauto[$entity_type] = (object)$settings;
      }
      return $pathauto;

    default:
      break;
  }
}

/**
 * Batch processing callback; Generate aliases for entities.
 */
function tourney_pathauto_bulk_update_batch_process(&$context, $entity_type) {
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

function tourney_match_pathauto_bulk_update_batch_process(&$context) {
  tourney_pathauto_bulk_update_batch_process($context, 'tourney_match');
}

function tourney_game_pathauto_bulk_update_batch_process(&$context) {
  tourney_pathauto_bulk_update_batch_process($context, 'tourney_game');
}

function tourney_tournament_pathauto_bulk_update_batch_process(&$context) {
  tourney_pathauto_bulk_update_batch_process($context, 'tourney_tournament');
}