<?php

function tourney_preprocess_tourney_tournament_view(&$vars) {
  drupal_add_css(drupal_get_path('module', 'tourney').'/theme/tourney.css');
  module_load_include('inc', 'tourney', 'theme/tourney');

  $matches = array();
  foreach ( $vars['view']->result as $result ) $matches[] = tourney_match_load($result);

  $rounds = array();
  foreach ( $matches as $match ) {
    $rounds[tourney_get_name_part($match->this_match, 'round')][$match->id] = array('match' => $match);
  }

  $vars['theme_variables'] = array(
    'tournament' => $matches[0]->tournament,
    'matches' => array(
      'bracket-' . tourney_get_name_part($matches[0]->this_match, 'bracket') => array_values($rounds),
    ),
  );
}
