<?php

/**
 * @file
 * A Ladder class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class Ladder implements TourneyMatch {
  public function __construct() {
    drupal_set_message('Ladder');
  }
}