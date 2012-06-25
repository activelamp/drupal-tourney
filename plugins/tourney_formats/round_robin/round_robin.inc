<?php

/**
 * @file
 * Default plugin for a default style.
 */

/**
 * Round Robin Format.
 */
$plugin = array(
  'title' => t('Round Robin'),
  'machine name' => 'roundrobin',
  'description' => t('This is the description of the tournament'),
  'weight' => 3,
  'total games' => 5,
  'controller' => 'RoundRobinController',
);

// Define any classes below to extend default functionality in the controllers.
// The default implementation works for this plugin.