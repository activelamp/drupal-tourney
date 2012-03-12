<?php

class tourney_handler_match_with_parents extends views_handler_filter {
  // exposed filter options
  var $always_multiple = TRUE;
  
  function option_definition() {
    $options = parent::option_definition();

    $options['operator']['default'] = 'specified_ancestors';
    $options['value']['default'] = array();
    $options['expose']['contains']['reduce'] = array('default' => FALSE);

    return $options;
  }
  
  /**
   * Provide simple equality operator
   */
  function operator_options() {
    return array(
      'specified_ancestors' => t('Specify generations up from'),
    );
  }
  
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['bracket'] = array(
      '#type' => 'select',
      '#title' => t('Bracket'),
      '#description' => t('Choose the bracket of match you are targeting'),
      '#options' => array(
        'bracket-top' => t('Bracket Top'),
        'bracket-bottom' => t('Bracket Bottom'),
        'bracket-champion' => t('Champion Bracket'),
      ),
      '#default_value' => -1,
    );
    $form['round'] = array(
      '#type' => 'select',
      '#title' => t('Round'),
      '#description' => t('Choose the round of match you are targeting'),
      '#options' => array(
        'round-1' => t('Round 1'),
        'round-2' => t('Round 2'),
        'round-3' => t('Round 3'),
        'round-4' => t('Round 4'),
        'round-5' => t('Round 5')
      ),
      '#default_value' => -1,
    );
    $form['number_of_generations'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of generations'),
      '#description' => t('Enter the number of generations to go back'),
      '#size' => 10,
      '#maxlength' => 255,
    );
    
    
  }
  
  /**
   * Provide a simple textfield for equality
   */
  function value_form(&$form, &$form_state) {
    $form['value'] = array(
      '#type' => 'textfield',
      '#title' => t('Name of Match'),
      '#size' => 30,
      '#default_value' => $this->value,
    );

    if (!empty($form_state['exposed'])) {
      $identifier = $this->options['expose']['identifier'];
      if (!isset($form_state['input'][$identifier])) {
        $form_state['input'][$identifier] = $this->value;
      }
    }
  }
}