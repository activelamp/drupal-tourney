<?php

/**
 * @file
 * Default plugin for a default style.
 */

/**
* Single Elimination Tournament
*/
$plugin = array(
  'name' => t('Single Elimination Tournament'),
  'machine name' => 'single_elim',
  'description' => t('This is the description of the tournament'),
  'weight' => 0,
  'total games' => 5,
  'builder controller' => 'SingleElimination', 
);

// Define any classes below to extend default functionality in the controllers.
// The default implementation works for this plugin.