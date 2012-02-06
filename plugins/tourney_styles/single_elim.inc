<?php
/**
* Single Elimination Tournament
*/
$plugin = array(
  'name' => t('Single Elimination Tournament'),
  'machine name' => 'single_elim',
  'description' => t('This is the description of the tournament'),
  'total games' => 5,
  'builder controller' => 'SingleElimination', 
  'completed controller' => 'SingleEliminationComplete',
);

// Define any classes below to extend default functionality.