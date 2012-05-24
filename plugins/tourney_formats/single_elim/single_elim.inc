<?php

/**
 * @file
 * Toureny format plugin for single elmination tournaments.
 */

/**
 * Single Elimination Format.
 */
$plugin = array(
  'name' => t('Single Elimination'),
  'machine name' => 'single',
  'description' => t('This is the description of the tournament'),
  'weight' => 0,
  'total games' => 5,
  'controller' => 'SingleEliminationController',
);
