<?php

/**
 * @file
 * A Double Elimination class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class DoubleElimination implements TourneyMatch {
  public function __construct() {
    drupal_set_message('Double elimination');
  }
}