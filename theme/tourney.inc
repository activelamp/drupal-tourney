<?php

/*
 * @file
 * Preprocess functions for all theme implementations provided by tourney.
 */
 
/**
 * 
 */
function template_preprocess_tourney_tournament_render(&$vars) {
  $plugin = &$vars['plugin'];
  
  // Initialize variables exposed in template.
  $vars['header'] = $vars['footer'] = $vars['matches'] = '';
  
  drupal_add_css(drupal_get_path('module', 'tourney') . '/theme/tourney.css');
  // Let the plugin preprocess variables
  $plugin->preprocess('tourney-tournament-render', $vars);
  
  // Build the tourney container classes.
  $vars['classes'] = join(' ', $vars['classes_array']);
}

/**
 * @todo: Think about whether or not I can move some of this logic to 
 *   TourneyController::preprocess()
 */
function template_preprocess_tourney_match_render(&$vars) {
  $mid = $vars['match']['id'];
  $plugin = &$vars['plugin'];
  $editable = FALSE;
  
  // Let the plugin preprocess variables
  $plugin->preprocess('tourney-match-render', $vars);
  
  // If we have a tournament object, load the match from tournament.
  if (is_object($plugin->tournament)) {
    $match = $plugin->tournament->getMatchById($mid);
  }
  
  // Get the contestants for the match, if match object exists.
  $contestants = !empty($match) && is_object($match) ? $match->getMatchContestants() : array();
  
  // If there is one contestants, pad the beginning if it's occupying slot 2
  if (count($contestants) == 1) {
    $first = current($contestants);
    if ($first->slot == 2) {
      // Fill in a NULL contestant in position 1.
      $contestants[1] = NULL;
      $contestants = array_reverse($contestants, TRUE);
      $editable = TRUE;
    }
  }
  // Fill in null contestants if two contestants are not assigned to match.
  $i = count($contestants);
  while (count($contestants) < 2) {
    $contestants[++$i] = (object) array('name' => NULL, 'id' => NULL);
    // This match is editable when at least one of the contestants is NULL.
    $editable = TRUE;
  }
  
  // Get the contestant output from the contestant template
  $contestant_output = '';
  $seeds = isset($vars['match']['seeds']) ? $vars['match']['seeds'] : '';
  foreach (array_values($contestants) as $slot => $contestant) {
    $slot++;
    if (!is_object($contestant)) {
      $contestant = (object) $contestant;
    }
    $contestant->slot = !empty($contestant->slot) ? $contestant->slot : $slot;
    $contestant_output .= theme('tourney_contestant', array(
      'contestant' => $contestant, 
      'seed' => isset($seeds[$slot]) ? $seeds[$slot] : NULL,
      'plugin' => $plugin,
      'winner' => property_exists($contestant, 'eid') && $contestant->eid == $match->winner,
    ));
  }
  
  // If we have a tournament object, get the match url from the plugin.
  $tournament = $plugin->tournament;
  if (is_object($tournament) && get_class($tournament) == 'TourneyTournamentEntity') {
    $match_uri = $plugin->getMatchURI($mid);
    if ($match_uri) {
      $editable = $editable == TRUE && $plugin->canSetContestants($mid);
      $contestant_output = l($contestant_output, $match_uri . ($editable ? "/edit" : ''), array(
        'html' => TRUE,
        'attributes' => array('class' => 'match-link'),
        'query' => $editable ? array('destination' => $match_uri) : array(),
      ));
    }
  }
  
  $vars['is_bye'] = isset($vars['match']['bye']) && $vars['match']['bye'] == TRUE;
  $vars['contestants'] = $contestant_output;
  $vars['has_children'] = isset($vars['match']['children']) && count($vars['match']['children']);
}

/**
 * 
 */
function template_preprocess_tourney_contestant(&$vars) {
  $plugin = &$vars['plugin'];
  
  $plugin->getPluginOptions();
  $options = $plugin->pluginOptions;
  
  $truncate = !empty($options['truncate']) ? $options['truncate'] : TOURNEY_TRUNCATE;
  
  // Let the plugin preprocess variables
  $plugin->preprocess('tourney-contestant', $vars);
  
  $vars['classes_array'][] = 'contestant-' . $vars['contestant']->slot;
  $vars['classes_array'][] = $vars['winner'] ? 'winner' : '';

  if (property_exists($vars['contestant'], 'name') && $vars['contestant']->name) {
    $vars['name'] = $vars['contestant']->name;
    $vars['classes_array'][] = 'eid-' . str_replace('.', '-', $vars['contestant']->eid);
  }
  else {
    $vars['name'] = 'Team ' . $vars['contestant']->slot;
  }
  
  if (strlen($vars['name']) > $truncate) {
    $vars['label'] = substr($vars['name'], 0, $truncate) . '...';
  }
  else {
    $vars['label'] = $vars['name'];
  }
  
  // Build the tourney container classes.
  $vars['classes'] = join(' ', $vars['classes_array']);
}