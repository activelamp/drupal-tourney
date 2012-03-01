<?php

function tourney_load_matches($ids, $total) {
  $matches = entity_load('tourney_game', $ids);
  while ( count($matches) < $total ) $matches[] = null;
  return $matches;
}

function tourney_matches_to_table($match_list) {
  $header = array();
  $rows = array(); 
  $mn = 0;
  $points = array();
 
  $header[] = 'Team Name';
  foreach ( $match_list as $match ) {
    $header[] = 'Game ' . ++$mn; 
    if ( $match != null ) {
      foreach ( $match->contestants as $id => $contestant ) {
        if ( !array_key_exists($id, $rows) ) {
          $rows[$id] = array($contestant);
          $points[$id] = 0;
        }
        if ( $match->winner == '' ) {
          $rows[$id][] = '-';
        }
        elseif ( $match->winner == $id ) {
          $rows[$id][] = 'W';
          $points[$id]++;
        }
        else {
          $rows[$id][] = 'L';
        }
      }
    }
    else {
      foreach ( $rows as $k => $v ) {
        $rows[$k][] = '-';
      }
    }
  }
  
  $winner = -1;
  foreach ( $points as $k => $v ) if ( $v > count($match_list)/2 )
    if ( $winner == -1 || $v > $points[$winner] ) $winner = $k;
  if ( $winner > -1 ) {
    $header[] = 'Results';
    foreach ( $rows as $k => $row ) {
      $rows[$k][] = $k == $winner ? 'Winner' : '-';
    }
  }
      
  return theme('table', array(
    'header' => $header,
    'rows' => $rows,
  ));
}

function theme_tourney_render_match($vars) {
  $match = $vars['match'];
  $matches = tourney_load_matches($match->games_played, $match->games);
  $table = tourney_matches_to_table($matches);
  return $table;
}
