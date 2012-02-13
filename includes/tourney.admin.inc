<?php

/**
 * @file
 * Tourney type editing UI.
 */

/**
 * UI controller.
 */
class TourneyTypeUIController extends EntityDefaultUIController {

  /**
   * Overrides hook_menu() defaults.
   */
  public function hook_menu() {
    $items = parent::hook_menu();
    $items[$this->path]['description'] = 'Manage profiles, including fields.';
    return $items;
  }
}

/**
 * Generates the profile type editing form.
 */
function tourney_type_form($form, &$form_state, $tourney_type, $op = 'edit') {
  if ($op == 'clone') {
    $tourney_type->label .= ' (cloned)';
    $tourney_type->type = '';
  }

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => isset($tourney_type->label) ? $tourney_type->label : '',
    '#description' => t('The human-readable name of this type.'),
    '#required' => TRUE,
    '#size' => 30,
  );
  // Machine-readable type name.
  $form['type'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($tourney_type->type) ? $tourney_type->type : '',
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => $form_state['entity_type'] .'_get_types',
      'source' => array('label'),
    ),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save tournament type'),
    '#weight' => 40,
  );
  
  if ($op != 'add' && $op != 'clone') {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete tournament type'),
      '#weight' => 45,
      '#limit_validation_errors' => array(),
      '#submit' => array('tourney_tournament_type_form_submit_delete')
    );
  }

  return $form;
}

/**
 * Form API submit callback for the type form.
 */
function tourney_tournament_type_form_submit(&$form, &$form_state) {
  $tourney_tournament_type = entity_ui_form_submit_build_entity($form, $form_state);
  // Save and go back.
  $tourney_tournament_type->save();
  $form_state['redirect'] = 'admin/structure/tournament';
}

/**
 * Form API submit callback for the delete button.
 */
function tourney_tournament_type_form_submit_delete(&$form, &$form_state) {
  $form_state['redirect'] = 'admin/structure/tournament/manage/' . $form_state['tourney_tournament_type']->type . '/delete';
}