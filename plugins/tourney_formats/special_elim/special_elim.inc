<?php

/**
 * @file
 * Tourney format plugin for special elmination tournaments.
 */

/**
 * Special Elimination Format.
 */
$plugin = array(
  'name' => t('Special Elimination'),
  'machine name' => 'special',
  'description' => t('This is the description of the tournament'),
  'weight' => 0,
  'total games' => 5,
  'controller' => 'SpecialEliminationController',
);