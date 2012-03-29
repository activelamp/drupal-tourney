<?php

class tourney_plugin_style_tournament extends views_plugin_style {
  function query() {
    parent::query();
    //dpm($this->view->query);
    //$this->view->query->field_aliases = array('tourney_tournament' => array('id'), 'tourney_match_tourney_tournament' => 'tourney_match_tourney_tournament_id');
  }
}