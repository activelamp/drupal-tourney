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
  // Set the tree node classes
  $node_classes = array();
  
  // @todo: There is probably a better way to find the path of this directory.
  // I don't like how this $plugin_dir variable is hard coded to this path,
  // need to figure out how to get the relative path of this file.
  $plugin_dir = drupal_get_path('module', 'tourney') . '/plugins/tourney_formats/single_elim';
  drupal_add_css($plugin_dir . '/theme/tournament-tree.css');
  
  // Use the very top of the tree if a child tree node is not passed in.
  if (!isset($vars['node'])) {
    $vars['node'] = $vars['plugin']->structure['tree'];
  }
  $node = $vars['node'];

  $is_bye = isset($node['bye']) ? $node['bye'] : FALSE;
  $is_child = isset($node['child']);
  $is_only_child = isset($node['only']) ? $node['only'] : FALSE;
  $has_children = isset($node['children']);
  
  // Check to see if the children of this match are byes or are both byes. Setting
  // a couple flags that will change if there are byes in the immediate child.
  // We need to know this so we can set the spacing properly in children matches.
  $has_bye = FALSE;
  $both_byes = TRUE;
  if ($has_children) {
    foreach ($node['children'] as &$child) {
      if (isset($child['bye']) && $child['bye']) {
        $has_bye = TRUE;
      }
      else {
        $both_byes = FALSE;
      }
    } 
    unset($child);
  }

  $bye_secondlevel = array();
  if ($has_children) {
    foreach ($node['children'] as $id => &$child) {
      $child['child'] = $id;
      $child['only']  = $has_bye && !$both_byes;
      if ($node['bracket'] == 'loser') {
        $child['child'] = count($node['children']) == 1 ? 1 : $id;
        $child['only']  = count($node['children']) == 1;
        $child['fix-height'] = TRUE;
      }
      if ($node['bracket'] == 'champion' && $node['round'] == 1) {
        $brackets = $vars['plugin']->data['brackets'];
        if ($id == 0) {
          $node_classes[] = 'no-tail-to-parent';
          $child['spacer'] = $brackets['loser']['rounds'] - $brackets['main']['rounds'];
        }
        elseif ($id == 1) {
          $child['loser-child'] = $brackets['loser']['rounds'] - $brackets['main']['rounds'];
        }
      }
      $child['bye_firstlevel'] = isset($node['bye_secondlevel']) ? $node['bye_secondlevel'] : array();
      $child['bye_secondlevel'] = &$bye_secondlevel;
      if (isset($child['children'])) {
        foreach ($child['children'] as $grandchild) {
          $bye_secondlevel[] = (isset($grandchild['bye']) && $grandchild['bye']);
        }
      }
    }
    unset($child);
  }
  $vars['node'] = $node;
  
  if ($is_bye) {
    $node_classes[] = 'bye';
  }       
  // If this branch of the tree has a bye in its first grandchild spot and not the
  // third spot, keep the height space in for the "tree node" match of the 
  // to compensate for the spacing of the third grandchild.
  if (isset($node['bye_firstlevel']) && $node['bye_firstlevel'] == array(TRUE, FALSE, FALSE, FALSE)) {
    if ($is_bye) {
      $node_classes[] = 'no-height-change';
    }
  }
  else {
    // Only childs always render on the bottom, add a class to push it to the bottom.
    if (!$is_bye && $is_only_child) {
      $node_classes[] = 'only-child';
    }
  }
  
  // Check for byes in the bottom bracket and make sure they render on the
  // bottom of the match block.
  if ($node['bracket'] == 'loser' && array_key_exists('bye', $node) 
    && $node['bye'] == FALSE && $node['bye_firstlevel'] == array(FALSE, TRUE)) {
    $node_classes[] = 'only-child';
    $vars['node']['child'] = 1;
  }
  
  
  if (!empty($node['loser-child'])) {
    $node_classes[] = 'loser-child-' . $node['loser-child'];
  }
  
  if (!empty($node['spacer'])) {
    $node_classes[] = 'round-space-' . $node['spacer'];
  }
  $vars['node_classes'] = implode(' ', $node_classes);
  
  // Set the children classes
  $children_classes = array();
  if ($both_byes) {
    $children_classes[] = 'bye';
  }
  $vars['children_classes'] = implode(' ', $children_classes);
  
  $vars['has_children'] = $has_children;
  $vars['is_child'] = $is_child;
}