<?php

class tourney_handler_field_rank extends views_handler_field {
  function construct() {
    parent::construct();
  }

  function query() {
    // Add the tournament id field
    $this->query->add_field('tourney_rank', 'tournament');
  }

  function render($values) {
    $tourney = $values->tourney_rank_tournament;
    $eid = $this->view->base_table . '.' . $values->{$this->view->base_field};
    
    return tourney_history_get_contestant_standing_change($tourney, $eid);
  }
}