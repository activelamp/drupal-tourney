<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class RoundRobin implements TourneyMatch{
  public function __construct() {
    drupal_set_message('Round Robin');
  }
}