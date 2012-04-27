<?php

/**
 * @file
 * Toureny format plugin for single elmination tournaments.
 */

/**
 * Single Elimination Tournament.
 */
$plugin = array(
  'name' => t('Single Elimination Tournament'),
  'machine name' => 'single_elim',
  'description' => t('This is the description of the tournament'),
  'weight' => 0,
  'total games' => 5,
  'builder controller' => 'SingleElimination',
);
