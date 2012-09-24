<?php

/**
 * @file
 * Tourney format plugin for special double elimination tournaments.
 */

/**
 * Special Double Elimination Format.
 */
$plugin = array(
  'title' => t('Double Elimination'),
  'machine name' => 'special',
  'description' => t('This is the description of the tournament'),
  'weight' => 2,
  'controller' => 'DoubleEliminationController',
);
