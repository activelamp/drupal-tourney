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
  'description' => t('Group tournaments together. This plugin provides no other logic'),
  'weight' => 0,
  // When we are ready to add base functionality, create the class below.
  'controller' => 'GenericTourneySeriesController',
);

class GenericTourneySeriesController extends TourneySeriesController {

}