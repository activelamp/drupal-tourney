<?php

class tourney_handler_match_tree extends views_handler_argument {
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['depth'] = array(
      '#title' => t('Depth'),
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

  function query() {
    module_load_include('inc', 'tourney', 'includes/tourney.match');
    module_load_include('inc', 'tourney', 'includes/tourney.tournament');
    $this->ensure_my_table();
    $ids = tourney_match_get_parents($this->argument, $this->options['depth']);
    $this->query->add_where(0, "$this->table_alias.$this->real_field", $ids);
    //dpm($this);
  }
}
