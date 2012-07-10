<?php

/**
 * @file
 * Tourney format plugin for a round robin tournament.
 */

/**
 * Round Robin Format.
 */
$plugin = array(
  'title' => t('Special Round Robin'),
  'machine name' => 'round_robin',
  'description' => t('This is the description of the tournament'),
  'weight' => 5,
  'controller' => 'SpecialRoundRobinController',
);