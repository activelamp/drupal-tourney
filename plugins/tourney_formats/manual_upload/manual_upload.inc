<?php

/**
 * @file
 * Tourney format plugin for a manual upload tournament.
 */

/**
 * Manual Upload Format.
 */
$plugin = array(
  'title' => t('Manual Upload'),
  'machine name' => 'manual_upload',
  'description' => t('This is the description of the tournament'),
  'weight' => 5,
  'controller' => 'ManualUploadController',
);

