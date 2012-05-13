<?php

/**
 * @file
 * Default plugin for a default style.
 */

/**
 * Ladder Format
 */
$plugin = array(
  'name' => t('Ladder'),
  'machine name' => 'ladder',
  'description' => t('This is the description of the tournament'),
  'weight' => 4,
  'total games' => 5,
  'controller' => 'LadderController',
);

// Define any classes below to extend default functionality in the controllers.
// The default implementation works for this plugin.