<?php

function tourney_render_tourney_single($tournament, $matches) {
  $rounds = array_values($matches['bracket-top']);
  $output = tourney_single_tree($rounds, $tournament->players > 8);
  return $output;
}