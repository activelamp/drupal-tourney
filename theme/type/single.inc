<?php

/*
 * @file
 */

function theme_tourney_render_tourney_single($vars) {
  extract($vars);
  drupal_add_js(drupal_get_path('module', 'tourney') . '/theme/js/single.js');
  $rounds = array_values($matches['bracket-top']);
  $output = theme('tourney_single_tree', array('rounds' => $rounds, 'small' => $tournament->players > 8));
  return $output;
}

function theme_tourney_single_tree($vars) {
  extract($vars); 
  $output = '';
  foreach ( $rounds as $r => $round ) {
    $last = $r >= count($rounds) -1;  // is this the last round?

    // If a round isn't set to render small, then the default rendering looks
    // squished (and it's impossible to display a title on top of each match).
    // Use the round 2 rendering to start this off, to add some padding.
    if (!$small) $r++;

    /*
     * To render each round, consider a team to be a row. To position the
     * matches, we render empty rows to pad the round out, then render the
     * two teams in the matchup, then render more empty rows to reach the
     * bottom. On a grid, the tournament renders like this:
     *
     * 1   -   -
     * 2   1   -
     * 3   4   -
     * 4   -   1
     * 5   -   8
     * 6   5   -
     * 7   8   -
     * 8   -   -
     *
     * Consider the amount of space each team takes up in each round. In the
     * first round, each player gets one row. In the next round, 50% of the
     * players have ben eliminated, so each player gets two rows. Their space
     * is multiplied by 2. In the next round again, their space is multiplied
     * by 2. Therefore, the formula to find their total space is 2^n
     * where n is the round number starting at 0. To calculate the empty row
     * padding, subtract one row (the one the player occupies). We will use
     * this padding heavily to render each round.
     *
     */
    $pad_rows = pow(2, $r)-1;

    $output .= '<div class="round round-' . $r . '">';
    foreach ( array_values($round) as $m => $match ) {

      // Group match pairs together. This gives the match more direct ownership
      // of its set of flow lines, and lets us target them more easily.
      if ( $m % 2 == 0 ) {
        // This match is even, so it's an "upper" match

        // Render empty rows before this match
        $output .= theme('tourney_spacer', array('height' => $pad_rows));

        // On even match, open the div. On odd, close it.
        $output .= '<div class="match-pair"><div class="match-contestant match-contestant-1">';

        // Render the match itself, and a flow tail coming off of it
        $match_flow_tail = ( $last ? '' : ( $pad_rows == 0 ? 'bottomleft evenright,special,bottom' : 'bottomleft' ) );
        $output .= theme('tourney_matchblock', array('match' => $match['match'], 'flow' => $match_flow_tail, 'small' => $small));

        if ($pad_rows && !$last) {
          // Render padding rows. These rows have flow lines in them.
          // The last padding row needs an evenright flow control, so we render
          // vertical flow rows up until the last one and then do one row with
          // the evenright.

          $output .= theme('tourney_spacer', array('height' => $pad_rows - 1, 'flow' => 'vertical'));
          $output .= theme('tourney_spacer', array('flow' => 'vertical evenright,special,bottom'));
        }
        $output .= '</div>'; // <!-- /match-contestant -->

        if ($last) {
          // Last round doesn't have a match pair, so close the match pair div.
          $output .= '</div>'; // <!-- /match-pair -->
        }
      }
      else {
        // This match is odd, so it's a "lower" match
        // Render rows with flow first, then the match, then empty spacing.

        $output .= '<div class="match-contestant match-contestant-2">';
        if ($pad_rows && !$last) {
          // Render padding rows with flow controls. Needs to start with 1
          // evenright, and then render the rest of the padding rows.
          $output .= theme('tourney_spacer', array('flow' => 'vertical evenright,special'));
          $output .= theme('tourney_spacer', array('height' => $pad_rows - 1, 'flow' => 'vertical'));
        }

        // Render the match, with a flow tail.
        $match_flow_tail = ( $last ? '' : ( $pad_rows == 0 ? 'topleft evenright,special' : 'topleft' ) );
        $output .= theme('tourney_matchblock', array('match' => $match['match'], 'flow' => $match_flow_tail, 'small' => $small));

        // Close match pair div
        $output .= '</div></div>';  // <!-- /match-contestant /match-pair -->

        // Empty padding to the bottom
        $output .= theme('tourney_spacer', array('height' => $pad_rows));
      }
    }
    $output .= '</div>';
  } 
  return $output;
}