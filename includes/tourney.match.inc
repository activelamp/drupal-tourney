<?php

/**
 * @file
 * Defines the wrapper functions for Entity API CRUD
 */
 
/**
 * Fetch a match object
 * 
 * @param $mid
 *   Integer specifying the match id
 * @param $reset
 *   A boolean indicating that the internal cache should be reset.
 * @return
 *   A fully-loaded object or FALSE of it cannot be loaded.
 * 
 * @see tourney_match_load_multiple()
 */
function tourney_match_load($mid = NULL, $reset = FALSE) {
  $mids = isset($mid) ? array($mid) : array();
  $conditions = array();
  $match = tourney_match_load_multiple($mids, $conditions, $reset);

  return $match ? reset($match) : FALSE;
}

/**
 * Load multiple match entities based on certain conditions.
 *
 * @param $mids
 *   An array of entity IDs.
 * @param $conditions
 *   An array of conditions to match against the {entity} table.
 * @param $reset
 *   A boolean indicating that the internal cache should be reset.
 * @return
 *   An array of match entity objects, indexed by mid.
 */
function tourney_match_load_multiple($mids = array(), $conditions = array(), $reset = FALSE) {
  return entity_load('tourney_match', $mids, $conditions, $reset);
}

/**
 * Delete multiple tournament entities.
 *
 * @param $mids
 *   An array of test entity IDs.
 */
function tourney_match_delete_multiple(array $mids) {
  entity_get_controller('tourney_match')->delete($mids);
}