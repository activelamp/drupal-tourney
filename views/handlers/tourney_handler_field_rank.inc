<?php

class tourney_handler_field_rank extends views_handler_field {
  function construct() {
    parent::construct();
  }

  function query() {

  }

  function render($values) {
    $tid = $this->view->args[0];
    return tourney_history_get_contestant_standing_change($tid, 'user.' . $values->uid);
  }
}