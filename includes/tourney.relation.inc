<?php

/**
 * @file
 * Relation related functions.
 */

/**
 * Get the tournament a match is related to.
 *
 * @param $match
 *   A match entity.
 *
 * @return
 *   A tournament entity.
 */
function tourney_get_tournament_by_match($match) {
  return relation_get_related_entity('tourney_match', $match->id, 'has_match', 1);
}

/**
 * Get the match a game is related to.
 *
 * @param $game
 *   A game entity.
 *
 * @return
 *   A match entity.
 */
function tourney_get_match_by_game($game) {
  return relation_get_related_entity('tourney_game', $game->id, 'has_game', 1);
}

/**
 * Get the tournament a game is related to.
 *
 * @param $game
 *   A game entity.
 *
 * @return
 *   A tournament entity.
 */
function tourney_get_tournament_by_game($game) {
  $match = relation_get_related_entity('tourney_game', $game->id, 'has_game', 1);
  return relation_get_related_entity('tourney_match', $match->id, 'has_match', 1);
}
