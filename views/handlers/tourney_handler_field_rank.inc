<?php

/**
 * @file
 * Custom views field handler for displaying the change in rank from 
 * tourney_history module.
 */

class tourney_handler_field_rank extends views_handler_field {
  function query() {
    // Add the tournament id field
    $this->query->add_field('tourney_rank', 'tournament');
  }

  function render($values) {
    $tourney = $values->tourney_rank_tournament;
    $eid = $this->view->base_table . '.' . $values->{$this->view->base_field};
  
    $change = esports_history_get_specific_change('team_ranks', $tourney, $eid);
    
    // Provide classes based on new position.
    if ($change > 0) {
      $rank_change = 'rank-down';
    }
    if ($change < 0) {
      $rank_change = 'rank-up';
    }
    else {
      $rank_change = 'rank-same';
    }
    
    return array(
      '#prefix' => '<span class="rank-change ' . $rank_change . '">',
      '#markup' => $change,
      '#suffix' => '</span>',
    );
  }
}