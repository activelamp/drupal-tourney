<?php

/**
 * @file
 * Tourney series plugin that has no logic.
 */

/**
 * Generic tournament series.
 */
$plugin = array(
  'title' => t('Generic series'),
  'machine name' => 'generic',
  'description' => t('This plugin provides no logic'),
  'weight' => 0,
  // When we are ready to add base functionality, create the class below.
  'controller' => 'GenericTourneySeriesController',
);
