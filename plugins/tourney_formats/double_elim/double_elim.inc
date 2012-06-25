<?php

/**
 * @file
 * Default plugin for a default style.
 */

/**
 * Double Elimination Format
 */
$plugin = array(
  'title' => t('Double Elimination'),
  'machine name' => 'double',
  'description' => t('This is the description of the tournament'),
  'weight' => 1,
  'total games' => 5,
  'controller' => 'DoubleEliminationController',
);

// Define any classes below to extend default functionality in the controllers.
// The default implementation works for this plugin.