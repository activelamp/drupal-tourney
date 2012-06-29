<?php

/**
 * @file
 * Tourney format plugin for single elimination tournaments.
 */

/**
 * Single Elimination Format.
 */
$plugin = array(
  'title' => t('Single Elimination'),
  'machine name' => 'single',
  'description' => t('This is the description of the tournament'),
  'weight' => 0,
  'controller' => 'SingleEliminationController',
);