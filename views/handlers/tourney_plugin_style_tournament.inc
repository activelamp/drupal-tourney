<?php

/**
 * @file
 */

class tourney_plugin_style_tournament extends views_plugin_style {
  function pre_render(&$values) {
    $args = $this->view->argument;
    if (!array_key_exists('match_tree', $args)) return;
    $mid = $args['match_tree']->argument;
    $depth = $args['match_tree']->options['depth'];
    if (array_key_exists('match_tree_depth', $args)) {
      if ($args['match_tree_depth']->argument != NULL) $depth = $args['match_tree_depth']->argument;
    }

    module_load_include('inc', 'tourney', 'includes/tourney.match');
    module_load_include('inc', 'tourney', 'includes/tourney.tournament');
    $values = tourney_match_get_parents($mid, $depth);
    sort($values);
  }

  function query() {
    $this->view->built = TRUE; 
    $this->view->executed = TRUE; 
  }
}