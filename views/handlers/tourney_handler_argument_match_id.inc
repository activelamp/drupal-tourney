<?php

/**
 * @file
 * Custom views argument that grabs the round id from plugin.
 */

class tourney_handler_argument_match_id extends views_handler_argument {
  function query($group_by = false) {}
   /**
    * Called to add the sort to a query.  Adds ISNULL() to the alias parameter.
    */
  function post_execute(&$results) {
    foreach ($results as $id => &$row) {
      $game = tourney_game_load($row->id);
      $match = $game->getMatch();
      $match->getMatchInfo();
      
      if ($match->matchInfo['round'] != $this->argument) {
        unset($this->view->result[$id]);
      }
    }
  }
}
