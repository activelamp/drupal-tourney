<?php

/*
 * @file
 * Preprocess tree nodes
 */  

/**
 * Preprocess each tree node in the bracket.  Take note that the use of $node in
 * this code IS NOT the $node object. $node is the "node" of the tree.
 */
function template_preprocess_tourney_tournament_tree_node(&$vars) {
  $plugin = &$vars['plugin'];
  drupal_add_js(drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/single_elim/theme/tournament-tree-node.js');
  // Set the tree node classes
  $node_classes = array();
  
  // Get any plugin options
  $plugin->getPluginOptions();
  $options = $plugin->pluginOptions;
  $options += array('show_byes' => NULL);

  // @todo: There is probably a better way to find the path of this directory.
  // I don't like how this $plugin_dir variable is hard coded to this path,
  // need to figure out how to get the relative path of this file.
  $plugin_dir = drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/single_elim';
  drupal_add_css($plugin_dir . '/theme/tournament-tree.css');
  
  // Use the very top of the tree if a child tree node is not passed in.
  if (!isset($vars['node'])) {
    $vars['node'] = $plugin->structure['tree'];
  }
  $node = &$vars['node'];

  $round_name = '';
  if (isset($options['show_round_names']) && $options['show_round_names']) {
    if ($node['roundMatch'] == 1) {
      if (is_object($plugin->tournament)) {
        $round_name = $plugin->tournament->get('round_' . $node['tourneyRound'], $plugin->data['rounds'][$node['tourneyRound']]['title']);
      }
      else {
        $round_name = $plugin->data['rounds'][$node['tourneyRound']]['title'];
      }
    }
  }

  $node_classes = array();
  $path_classes = array();
  $bye = (isset($node['bye'])) ? $node['bye'] : FALSE;
  $has_children = isset($node['children']) && count($node['children']);
  $is_child     = isset($node['child']);

  $node_classes[] = 'bracket-' . $node['bracket'];
  if ($node['bracket'] != 'main' && $has_children && count($node['children']) == 2) {
    $node_classes[] = 'feeder';
  } 

  if ($is_child) {
    $node_classes[] = 'child-' . $node['child'];
    if (isset($node['only_child']) && $node['only_child']) {
      $only_child = TRUE;
      $node_classes[] = 'only-child';
    }
  }
  else {
    $node_classes[] = 'top-node';
  }
  
  if ($has_children) {
    // This line is to correct rendering quirks that can arise because
    // bottom bracket matches are filled from the bottom up
    // if ($node['bracket'] == 'loser') $node['children'] = array_reverse($node['children']);
    $node_classes[] = 'children-' . count($node['children']);
    foreach ($node['children'] as $id => &$child) {
      if (count($node['children']) == 1) {
        $child['child'] = 1;
        $child['only_child'] = TRUE;
      }
      else {
        $child['child'] = $id;
      }
    }
  }

  if (is_object($plugin->tournament)) {
    $match = $plugin->tournament->getMatchById($node['id']);
    if ($match) {
      $match->matchInfo = $node;
      $parent_classes = array();

      if (array_key_exists('hide', $match->matchInfo) && $match->matchInfo['hide'] == TRUE) {
        $parent_classes[] = 'hide-match';
      }
      $vars['match'] = $match;
      if (is_object($match) && $match->winner) {
        $path_classes[] = 'eid-' . str_replace('.', '-', $match->winner);
        $contestants = array_keys($match->getMatchContestants());
        if ($match->winner != $contestants[0]) {
          $path_classes[] = 'winner-bottom';
        }
      }
    }
    else {
      $vars['match'] = null;
    }
  }

  $vars['round_name']       = $round_name;
  $vars['node_classes']     = implode(' ', $node_classes);
  $vars['path_classes']     = implode(' ', $path_classes);
  $vars['parent_classes']     = implode(' ', $parent_classes);
  $vars['bye']              = $bye;
  $vars['has_children']     = $has_children;
  $vars['round_name']       = '';
}