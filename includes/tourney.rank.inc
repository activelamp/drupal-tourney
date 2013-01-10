<?php

/**
 * @file
 * Tourney Rankings system.
 */

/**
 * Checks if a ranking for a specific contestant in a tournament exists
 *
 * @param $tournament
 *   Tournament entity
 * @param $contestant
 *   Contestant entity
 * @return boolean
 *   Ranking exists
 */
function tourney_rank_exists($tournament, $contestant) {
  $result = db_query('SELECT rank 
    FROM {tourney_rank} r 
    WHERE r.tournament = :tournament AND r.entity_type = :entity_type AND r.entity_id = :entity_id',
    array(':tournament' => $tournament->id, ':entity_type' => $contestant->entity_type, ':entity_id' => $contestant->entity_id
    ));
  $count = $result->rowCount();
  return $count > 0;
}

/**
 * Create a new rank entry
 *
 * @param $tournament
 *   Tournament entity
 * @param $contestant
 *   Contestant entity
 */
function tourney_rank_create($tournament, $contestant) {
  if (!$contestant) return FALSE;
  db_insert('tourney_rank')
    ->fields(array(
      'tournament' => $tournament->id,
      'entity_type' => $contestant->entity_type,
      'entity_id' => $contestant->entity_id,
      'wins' => 0,
      'losses' => 0,
      'rank' => 0,
    ))
    ->execute();
  return TRUE;
}

/**
 * Update a rank entry with a win
 *
 * @param $tournament
 *   Tournament entity
 * @param $contestant
 *   Contestant entity
 */
function tourney_rank_win($tournament, $contestant, $undo = FALSE) {
  if (!tourney_rank_exists($tournament, $contestant) && !tourney_rank_create($tournament, $contestant)) return;
  $query = db_update('tourney_rank');
  if (!$undo) {
    $query
      ->expression('wins', 'wins + 1')
      ->expression('rank', 'rank + .5');
  }
  else {
    $query
      ->expression('wins', 'wins - 1')
      ->expression('rank', 'rank - .5');
  }
  $query
    ->condition('tournament', $tournament->id)
    ->condition('entity_type', $contestant->entity_type)
    ->condition('entity_id', $contestant->entity_id)
    ->execute();
}

/**
 * Update a rank entry with a loss
 *
 * @param $tournament
 *   Tournament entity
 * @param $contestant
 *   Contestant entity
 */
function tourney_rank_lose($tournament, $contestant, $undo = FALSE) {
  if (!tourney_rank_exists($tournament, $contestant) && !tourney_rank_create($tournament, $contestant)) return;
  $query = db_update('tourney_rank');
  if (!$undo) {
    $query
      ->expression('losses', 'losses + 1')
      ->expression('rank', 'rank - .5');
  }
  else {
    $query
      ->expression('losses', 'losses - 1')
      ->expression('rank', 'rank + .5');
  }
  $query
    ->condition('tournament', $tournament->id)
    ->condition('entity_type', $contestant->entity_type)
    ->condition('entity_id', $contestant->entity_id)
    ->execute();
}

function tourney_rank_reset($tournament) {
  db_query('DELETE FROM {tourney_rank}
    WHERE tournament = :tournament',
    array(':tournament' => $tournament->id
    ));
  $game_ids = $tournament->getGameIds();
  $games = entity_load('tourney_game', $game_ids);
  foreach ($games as $game) {
    $contestants = $game->getContestants();
    if ($game->winner) {
      tourney_rank_win($tournament, $contestants[$game->winner]);
      unset($contestants[$game->winner]);
      $loser = reset($contestants);
      tourney_rank_lose($tournament, $loser);
    }
  }
  return '';
}