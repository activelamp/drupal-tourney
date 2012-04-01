<?php

function tourney_render_tourney_double($tournament, $matches) {
  $output = '';

  if ( array_key_exists('bracket-top', $matches) )
    $output  = '<div class="bracket bracket-top">' 
            . tourney_dummy_rounds(count($matches['bracket-bottom']) - count($matches['bracket-top']), $tournament->players)
            . tourney_double_top_bracket(array_values($matches['bracket-top']), $tournament->players) . '</div>';
  if ( array_key_exists('bracket-champion', $matches) )
    $output .= '<div class="bracket bracket-champion">' . tourney_double_champion_bracket($tournament, array_values($matches['bracket-champion']), 1) . '</div>';
  if ( array_key_exists('bracket-bottom', $matches) )
    $output .= '<div class="bracket bracket-bottom">' . tourney_double_bottom_bracket(array_values($matches['bracket-bottom'])) . '</div>';
  return $output;
}

function tourney_dummy_rounds($count, $height, $small = 1) {
  $round = '<div class="round round-dummy">' . tourney_spacer($height, 0, $small) . '</div>';
  return str_repeat($round, $count);
}


function tourney_double_top_bracket($rounds) {
  // Note: copied with remarkable shamelessness from tourney_single_tree().
  // One alteration is rendering an extra flow line from the last round to
  // the championship bracket.

  $small = TRUE;
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
        $output .= tourney_spacer($pad_rows, null, TRUE);

        // On even match, open the div. On odd, close it.
        $output .= '<div class="match-pair"><div class="match-contestant match-contestant-1">';

        // Render the match itself, and a flow tail coming off of it
        $match_flow_tail = ( $last ? '' : ( $pad_rows == 0 ? 'bottomleft evenright,special,bottom' : 'bottomleft' ) );
        $output .= tourney_matchblock($match['match'], $match_flow_tail, $small);

        if ($pad_rows) {
          if (!$last) {
            // Render padding rows. These rows have flow lines in them.
            // The last padding row needs an evenright flow control, so we render
            // vertical flow rows up until the last one and then do one row with
            // the evenright.
            $output .= tourney_spacer($pad_rows - 1, 'vertical', TRUE);
            $output .= tourney_spacer(1, 'vertical evenright,special,bottom', TRUE);
          }
          else {
            // On the last round, render an extra long flow line to the
            // championship match. We goin to the Super Bowl, baby!
            // @todo
          }
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
        if ($pad_rows) {
          if (!$last) {
            // Render padding rows with flow controls. Needs to start with 1
            // evenright, and then render the rest of the padding rows.
            $output .= tourney_spacer(1, 'vertical evenright,special', TRUE);
            $output .= tourney_spacer($pad_rows - 1, 'vertical', TRUE);
          }
          else {
            // Render flow line to the championship match.
            // @todo
          }
        }

        // Render the match, with a flow tail.
        $match_flow_tail = ( $last ? '' : ( $pad_rows == 0 ? 'topleft evenright,special' : 'topleft' ) );
        $output .= tourney_matchblock($match['match'], $match_flow_tail, $small);

        // Close match pair div
        $output .= '</div></div>';  // <!-- /match-contestant /match-pair -->

        // Empty padding to the bottom
        $output .= tourney_spacer($pad_rows, null, TRUE);
      }
    }
    $output .= '</div>';
  }
  return $output;
}

function tourney_double_bottom_bracket($rounds) {
  $small = TRUE;
  $output = '';

  foreach ( $rounds as $r => $round ) {
    /*
     * The bottom bracket is a little more complicated to explain than the top
     * bracket. The main difference is that each even round (0, 2, 4) feeds
     * into a round with the same number of players. Losers are added on odd
     * rounds, so rounds 0 and 1 both have 8 players, rounds 2 and 3 both have
     * 4 players, etc. Because of this, we can use the padding formula from the
     * top bracket for spacing, but with one change. We have to subtract one
     * padding from the top of the round each time losers are added. Also, we
     * use an initial padding of 1 to account for the first padding subtraction.
     *
     * -   1   -   1   -   1
     * 1   9   1   D   -   F
     * 2   A   4   -   1   -
     * 3   4   -   -   8   -
     * 4   5   -   E   -   -
     * 5   B   5   8   -   -
     * 6   C   8   -   -   -
     * 7   8   -   -   -   -
     * 8   -   -   -   -   -
     *
     *
     * So to put it all together:
     * effective round number = floor(round number / 2)
     * padding rows = (effective round number)^2 - 1.
     * top padding = 1 (initial pad) + padding rows - effective round number
     * If n is odd then top padding = top padding - 1.
     *
     * Additionaly, even rounds render all matches with "lower match" flow
     * lines.
     */
    $cr = floor($r/2);
    $feed_round = ($r % 2 == 0);
    $clast = $cr >= floor((count($rounds) - 1) / 2);  // is this the last round?
    $last = $r >= count($rounds) - 1;  // is this the absolute last round?
    

    $pad_rows = pow(2, $cr)-1;
    $top_pad = 1 + $pad_rows - $cr;
    if (!$feed_round) $top_pad--;

    $output .= '<div class="round round-' . $r . '">';

    foreach ( array_values($round) as $m => $match ) {

      // Group match pairs together. This gives the match more direct ownership
      // of its set of flow lines, and lets us target them more easily.
      if ( $m % 2 == 0 ) {
        // This match is even, so it's an "upper" match

        if ($m == 0) {
          // If this is the first match of the round, use the top padding
          // number to render padding
          $output .= tourney_spacer($top_pad, null, TRUE);
        }
        else {
          // Render empty rows before this match
          $output .= tourney_spacer($pad_rows, null, TRUE);
        }

        // On even match, open the div. On odd, close it.
        $output .= '<div class="match-pair"><div class="match-contestant match-contestant-1">';

        // Render the match itself, and a flow tail coming off of it
        if ($feed_round) {
          $match_flow_tail = ( $last ? '' : 'topleft evenright,special' );
        }
        else {
          $match_flow_tail = ( $last ? '' : ( $pad_rows == 0 ? 'bottomleft evenright,special,bottom' : 'bottomleft' ) );
        }
        $output .= tourney_matchblock($match['match'], $match_flow_tail, $small);

        if ($pad_rows) {
          if (!$last) {
            if ($feed_round) {
              // On feed rounds (rounds that feed into a round with the same
              // number of contestants), we still need padding, but we want
              // blank space instead of flow lines.
              $output .= tourney_spacer($pad_rows, null, TRUE);
            }
            else {
              // Render padding rows. These rows have flow lines in them.
              // The last padding row needs an evenright flow control, so we render
              // vertical flow rows up until the last one and then do one row with
              // the evenright.
              $output .= tourney_spacer($pad_rows - 1, 'vertical', TRUE);
              $output .= tourney_spacer(1, 'vertical evenright,special,bottom', TRUE);
            }
          }
          else {
            // Render flow line to the championship match.
            // @todo
          }
        }
        $output .= '</div>'; // <!-- /match-contestant -->

        if ($clast) {
          // Last round doesn't have a match pair, so close the match pair div.
          $output .= '</div>'; // <!-- /match-pair -->
        }
      }
      else {
        // This match is odd, so it's a "lower" match
        // Render rows with flow first, then the match, then empty spacing.

        $output .= '<div class="match-contestant match-contestant-2">';
        if ($pad_rows) {
          if (!$last) {
            if ($feed_round) {
              // On feed rounds (rounds that feed into a round with the same
              // number of contestants), we still need padding, but we want
              // blank space instead of flow lines.
              $output .= tourney_spacer($pad_rows, null, TRUE);
            }
            else {
              // Render padding rows with flow controls. Needs to start with 1
              // evenright, and then render the rest of the padding rows.
              $output .= tourney_spacer(1, 'vertical evenright,special', TRUE);
              $output .= tourney_spacer($pad_rows - 1, 'vertical', TRUE);
            }
          }
          else {
            // Render flow line to the championship match.
            // @todo
          }
        }

        // Render the match, with a flow tail.
        if ($feed_round) {
          $match_flow_tail = ( $last ? '' : 'topleft evenright,special' );
        }
        else {
          $match_flow_tail = ( $last ? '' : ( $pad_rows == 0 ? 'topleft evenright,special' : 'topleft' ) );
        }
        $output .= tourney_matchblock($match['match'], $match_flow_tail, $small);

        // Close match pair div
        $output .= '</div></div>';  // <!-- /match-contestant /match-pair -->

        // Empty padding to the bottom
        $output .= tourney_spacer($pad_rows, null, TRUE);
      }
    }
    $output .= '</div>';
  } 
  return $output;
}

function tourney_double_champion_bracket($tournament, $rounds) {
  $champ = array_pop($rounds[0]);
  $champ = $champ['match'];

  $output  = '';
  $output .= '<div class="round round-champion">';
  $output .= tourney_spacer(3 * ($tournament->players / 4)-1, null, 1);
  $output .= tourney_matchblock($champ, 'horizontal', 1);
  $output .= '</div>';
  $output .= '<div class="round round-extra">';
  $output .= tourney_spacer(3 * ($tournament->players / 4)-1, null, 1);
  $output .= tourney_matchblock($champ, null, 1);
  $output .= '</div>';
  return $output;
}