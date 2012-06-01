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
        //$settings['batch_update_callback'] = 'node_pathauto_bulk_update_batch_process';
        //$settings['batch_file'] = drupal_get_path('module', 'pathauto') . '/pathauto.pathauto.inc';

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
      break;

    default:
      break;
  }
}
