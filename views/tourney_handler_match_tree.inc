<?php

/**
 * @file
 */

class tourney_handler_match_tree extends views_handler_argument {
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['depth'] = array(
      '#title' => t('Default Depth'),
      '#description' => 'This can be overridden by providing a Match Tree Depth contextual filter',
      '#type' => 'textfield',
      '#default_value' => $this->options['depth'],
      '#fieldset' => 'argument_present',
    );
  }
  function option_definition() {
    $options = parent::option_definition();
    $options['depth'] = array('default' => -1);
    return $options;
  }
}
