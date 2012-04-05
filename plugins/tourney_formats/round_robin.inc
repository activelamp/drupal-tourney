<?php

/**
 * @file
 * Default plugin for a default style.
 */

/**
 * Round Robin Tournament
 */
$plugin = array(
  'name' => t('Round Robin Tournament'),
  'machine name' => 'round_robin',
  'description' => t('This is the description of the tournament'),
  'weight' => 3,
  'total games' => 5,
  'builder controller' => 'RoundRobin',
);

// Define any classes below to extend default functionality in the controllers.
// The default implementation works for this plugin.