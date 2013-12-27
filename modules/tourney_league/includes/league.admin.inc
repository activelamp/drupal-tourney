<?php

/**
 * @file
 * Admin callbacks for league entities..
 */

/**
 * Returns content that drupal can use to render a page
 */
function tourney_league_view_page($league, $view_mode = 'full') {
  $league->content = array();
  field_attach_prepare_view('league', array($league->id => $league), $view_mode);
  entity_prepare_view('league', array($league->id => $league));
  $league->content += field_attach_view('league', $league, $view_mode);
  return $league->content;
}

/**
 * Returns a custom title
 */
function tourney_league_page_title($league) {
  return "$league->entity_name ($league->id)";
}

/**
 * Delete page callback
 */
function tourney_league_confirm_delete_page($form, &$form_state, $league) {
  $form = array();

  $form["#league"] = $league;
  return confirm_form(
    $form,
    t('Are you sure you want to delete the league %title?', array('%title' => $league->label)),
    "tourney/league/$league->id",
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel'),
    'tourney_league_confirm_delete_page_submit'
  );
}


/**
 * Returns the information of a bundle for the league entity
 */
function tourney_league_types_information($league_type) {
  return $league_type->type . ': ' . $league_type->description;
}

/**
 * Form defintion for the league forms
 */
function tourney_league_form($form, &$form_state, $league) {
  $form['#id'] = 'league-form';

  // store the league for later
  $form['#league'] = $league;
  $form_state['league'] = $league;

  // label field
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('League Name'),
    '#default_value' => isset($league->label) ? $league->label : '',
    '#weight' => -5,
    '#required' => TRUE,
  );

  $form['additional_settings'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => 99,
  );

  $form['path'] = tourney_create_path_field($league);
//  $form['menu'] = tourney_league_create_menu();

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Publishing Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#group' => 'additional_settings',
    '#weight' => 95,
  );

  $form['options']['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Published'),
    '#default_value' => isset($league->status) ? $league->status : 0,
  );

  // buttons
  $form['buttons'] = array();
  $form['buttons']['#weight'] = 100;
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('tourney_league_form_submit'),
  );

  if (!empty($league->id)) {
   $form['buttons']['delete'] = array(
     '#access' => user_access('delete league'),
     '#type' => 'submit',
     '#value' => t('Delete'),
     '#weight' => 15,
     '#submit' => array('league_form_delete_submit'),
   );
  }

  $form['#validate'][] = 'tourney_league_form_validate';
  field_attach_form('tourney_league', $league, $form, $form_state);

  return $form;
}

function tourney_league_create_menu(&$form) {
  // Set default values.
  $link = array(
    'link_title' => '',
    'mlid' => 0,
    'plid' => 0,
    'menu_name' => $menu_name,
    'weight' => 0,
    'options' => array(),
    'module' => 'menu',
    'expanded' => 0,
    'hidden' => 0,
    'has_children' => 0,
    'customized' => 0,
  );
  $type = array('mlid' => 0);
  $options = menu_parent_options(menu_get_menus(), $type, array());
  $form['menu'] = array(
    '#type' => 'fieldset',
    '#title' => t('Menu settings'),
    '#access' => user_access('administer menu'),
    '#collapsible' => TRUE,
    '#collapsed' => !$link['link_title'],
    '#group' => 'additional_settings',
    '#attached' => array(
      'js' => array(drupal_get_path('module', 'menu') . '/menu.js'),
    ),
    '#tree' => TRUE,
    '#weight' => -2,
    '#attributes' => array('class' => array('menu-link-form')),
  );
  $form['menu']['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Provide a menu link'),
    '#default_value' => (int) (bool) $link['mlid'],
  );
  $form['menu']['link'] = array(
    '#type' => 'container',
    '#parents' => array('menu'),
    '#states' => array(
      'invisible' => array(
        'input[name="menu[enabled]"]' => array('checked' => FALSE),
      ),
    ),
  );

  // Populate the element with the link data.
  foreach (array('mlid', 'module', 'hidden', 'has_children', 'customized', 'options', 'expanded', 'hidden', 'parent_depth_limit') as $key) {
    $form['menu']['link'][$key] = array('#type' => 'value', '#value' => $link[$key]);
  }

  $form['menu']['link']['link_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Menu link title'),
    '#default_value' => $link['link_title'],
  );

  $form['menu']['link']['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => isset($link['options']['attributes']['title']) ? $link['options']['attributes']['title'] : '',
    '#rows' => 1,
    '#description' => t('Shown when hovering over the menu link.'),
  );

//  $default = ($link['mlid'] ? $link['menu_name'] . ':' . $link['plid'] : variable_get('menu_parent_' . $type, 'main-menu:0'));
  // If the current parent menu item is not present in options, use the first
  // available option as default value.
  // @todo User should not be allowed to access menu link settings in such a
  // case.
  if (!isset($options[$default])) {
    $array = array_keys($options);
    $default = reset($array);
  }
  $form['menu']['link']['parent'] = array(
    '#type' => 'select',
    '#title' => t('Parent item'),
    '#default_value' => $default,
    '#options' => $options,
    '#attributes' => array('class' => array('menu-parent-select')),
  );
  $form['menu']['link']['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#delta' => 50,
    '#default_value' => $link['weight'],
    '#description' => t('Menu links with smaller weights are displayed before links with larger weights.'),
  );

  return $form['menu'];
}

/**
 * Helper for hook_node_insert() and hook_node_update().
 */
function tourney_league_menu_save($league) {
  if (isset($league->menu)) {
    $link = &$league->menu;
    if (empty($link['enabled'])) {
      if (!empty($link['mlid'])) {
        menu_link_delete($link['mlid']);
      }
    }
    elseif (trim($link['link_title'])) {
      $link['link_title'] = trim($link['link_title']);
      $link['link_path'] = "tourney/league/$league->id";
      if (trim($link['description'])) {
        $link['options']['attributes']['title'] = trim($link['description']);
      }
      else {
        // If the description field was left empty, remove the title attribute
        // from the menu link.
        unset($link['options']['attributes']['title']);
      }
      if (!menu_link_save($link)) {
        drupal_set_message(t('There was an error saving the menu link.'), 'error');
      }
    }
  }
}

/**
 * Validate hook for league forms
 */
function tourney_league_form_validate($form, &$form_state) {
  $league = $form_state['league'];
  field_attach_form_validate('tourney_league', $league, $form, $form_state);
}

/**
 * Implements league_form_submit
 */
function tourney_league_form_submit($form, &$form_state) {
  $league = &$form_state['league'];

  if (!empty($form_state['values']['menu']['parent'])) {
    list($league->menu['menu_name'], $league->menu['plid']) = explode(':', $form_state['values']['menu']['parent']);
  }

  entity_form_submit_build_entity('tourney_league', $league, $form, $form_state);
  $league->save();
  tourney_league_menu_save($league);
  $form_state['redirect'] = 'tourney/league/' . $league->id;
}

/**
 * Implements league_form_delete_submit
 */
function league_form_delete_submit($form, &$form_state) {
  $form_state['redirect'] = 'tourney/league/' . $form_state['league']->id. '/delete';
}

/**
 * Show the tournament page.
 */
function tourney_league_page_view($tourney, $view_mode = 'full') {
  return $tourney->view($view_mode, NULL, TRUE);
}

/**
 * Page callback. Edit a tournament.
 */
function tourney_league_edit($league) {
  drupal_set_title(t('Edit League: @title', array('@title' => $league->label)));
  return drupal_get_form('tourney_league_form', $league);
}

/**
 * Delete submit callback.
 */
function tourney_league_confirm_delete_page_submit($form, &$form_state) {
  entity_delete('tourney_league', $form['#league']->id);
  $form_state['redirect'] = "admin/content/league";
}
