<?php

/**
 * @file
 */

function tourney_roundrobin_standings_sort($a, $b) {
  if ( $a['wins'] != $b['wins'] ) {
    return $a['wins'] < $b['wins'] ? 1 : -1;
  }
  elseif ( $a['losses'] != $b['losses'] ) {
    return $a['losses'] < $b['losses'] ? 1 : -1;
  }
  else {
    return $a['first'] < $b['first'] ? 1 : -1;
  }
}

function theme_tourney_roundrobin($vars) {
  $tournament = $vars['tournament'];

  // Render the standings table.
  $output = theme('tourney_roundrobin_standings', array(
    'tournament' => $tournament
  ));
  
  // $matches = $tournament->buildMatches();
  foreach ($tournament->data['bracket-roundrobin']['rounds'] as $round_name => $round) {
    $r = substr(strstr($round_name, '-'), 1);
    $output .= "<h3 id='round-$r'>Round $r</h3>";
    foreach ($round['matches'] as $match_name => $match_callbacks) {
      $match = $tournament->tourneyFormatPlugin
        ->$match_callbacks['current_match']['callback']($match_callbacks['current_match']['args']);
        
      $output .= theme('tourney_matchblock', array('match' => $match));
    } 
    $output .= '<div class="clearfix"></div>';
  }
  return $output;
}

/**
 * Renders the html for the leader board for a round robin tourney
 */
function theme_tourney_roundrobin_standings($vars) {
  $tournament = $vars['tournament'];
  return;
  $matches = $tournament->buildMatches();
  
  $contestants = $tournament->getContestants();
  $standings = $tournament->getStandings();
  
  $table['header'] = array('Teams', 'Games Played', 'Wins', 'Losses');
  $table['rows'] = array();
  
  uasort($standings, 'tourney_roundrobin_standings_sort');

  foreach ( $standings as $id => $standing ) {
    $table['rows'][] = array($contestants[$id]->entity_name, $standing['total'], $standing['wins'], $standing['losses']);
  }

  return theme('table', $table);
}