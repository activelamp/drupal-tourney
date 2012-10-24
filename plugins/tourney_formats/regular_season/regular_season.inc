<?php

/**
 * @file
 * Tourney format plugin for regular season tournaments.
 */

/**
 * Regular Season Format.
 */
$plugin = array(
  'title' => t('Regular Season'),
  'machine name' => 'regular',
  'description' => t('A regular season tournament'),
  'weight' => 0,
  'controller' => 'RegularSeasonController',
);
