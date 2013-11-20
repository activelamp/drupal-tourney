<?php

/**
 * UI controller.
 */
class LeagueTypeUIController extends EntityDefaultUIController {

  /**
   * Overrides hook_menu() defaults.
   */
  public function hook_menu() {
    $items = parent::hook_menu();
    $items[$this->path]['description'] = 'Manage leagues, including fields.';
    return $items;
  }
}


/**
 * Generates the league type editing form.
 */
function tourney_league_type_form($form, &$form_state, $league_type, $op = 'edit') {
  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => !empty($league_type->label) ? $league_type->label : '',
    '#description' => t('The human-readable name of this league type.'),
    '#required' => TRUE,
    '#size' => 30,
  );
  // Machine-readable type name.
  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($league_type->type) ? $league_type->type : '',
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => 'tourney_league_get_types',
      'source' => array('label'),
    ),
    '#description' => t('A unique machine-readable name for this league type. It must only contain lowercase letters, numbers, and underscores.'),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save league type'),
    '#weight' => 40,
  );

  if ($op != 'add' && $op != 'clone') {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete league type'),
      '#weight' => 45,
      '#limit_validation_errors' => array(),
      '#submit' => array('tourney_league_type_form_submit_delete')
    );
  }
  return $form;
}

/**
 * Form API submit callback for the type form.
 */
function tourney_league_type_form_submit(&$form, &$form_state) {
  $entity = entity_ui_form_submit_build_entity($form, $form_state);
  // Save and go back.
  $entity->save();
  $form_state['redirect'] = 'admin/structure/leagues';
}

/**
 * Form API submit callback for the delete button.
 */
function tourney_league_type_form_submit_delete(&$form, &$form_state) {
  $form_state['redirect'] = 'admin/structure/leagues/manage/' . $form_state['playerstats_type']->type . '/delete';
}