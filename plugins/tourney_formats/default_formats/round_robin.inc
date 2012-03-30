<?php

/**
 * @file
 * A Round Robin class for tournaments.
 */
 
/**
 * A class defining how matches are created for this style tournament.
 */
class RoundRobin implements TourneyMatchInterface {
  
  protected $num_contestants;
  protected $matches;
  
  /**
   * Constructor
   */
  public function __construct($num_contestants) {
    $this->num_contestants = $num_contestants;
  }
  
  /**
   * Build an array with all possible matches.
   * @see http://www.ehow.com/how_5796594_create-round_robin-schedule.html
   * 
   * With round robin tournaments the teams switch off between being the "home"
   * team and the "away" team.  To accomodate this, assume the following is true:
   *   HOME TEAM = 'contestant-1'
   *   AWAY TEAM = 'contestant-2'
   * 
   * There is a specific order that the matches are played. The fields that
   * track movement were initially built for winner path and loser path. In
   * round robin tournaments the path the teams take in the tournament do not
   * depend on who wins the match, it is already determined at the start of
   * the tournament. To accomodate for this, assume the following is true:
   *   CONTESTANT 1 NEXT GAME = next_match_winner
   *   CONTESTANT 2 NEXT GAME = next_match_loser
   * 
   * @todo Fix the name of the paths so that they are named more generic.
   */
  public function build() {
    // Ensure we have an even number of spots
    $spots = $this->num_contestants % 2 ? $this->num_contestants + 1 : $this->num_contestants;
    // Number of rounds is (n - 1), n being contestants
    $rounds = $spots - 1;
    
    $matches = array();
    
    for ($r=1;$r<=$rounds;$r++) {
      for ($m=1;$m<=$spots/2;$m++) {
        $matches['round-'. $r]['match-'. $m]['contestant-1'] = $r == 1 ? 'manual' : '';
        $matches['round-'. $r]['match-'. $m]['contestant-2'] = $r == 1 ? 'manual' : '';
        
        $matches = $this->set_match_path($matches, $r, $m);
      }
    }
    dpm($matches);
    $this->matches = $matches;
    return $this->matches;
  }
  
  /**
   * Figure out where each team needs to go next based on standard round robin
   * logic. Build the array that determines the spots.
   */
  private function placeholders() {
    $num_rounds = $num_teams - 1; 
    $rounds = array(); 

    for($round = 0; $round < $num_rounds; $round++) {       
      for ($index = 0; $index < $num_teams / 2; $index++) { 

        // Create the ascending array value
        $local_key = ($index != 0) * ($index - $round) + 
          (($index != 0) && (($index != 0) * ($index - $round) <= 0)) * $num_rounds; 

        // Create the descending array value
        $away_key = $num_rounds - $index - $round + 
          (($index != 0) && ($num_rounds - $index - $round <= 0)) * $num_rounds; 

        // The home team and visitor team switch every other game
        $rounds[$round][] = $round % 2 ? array($away_key, $local_key) : array($local_key, $away_key); 
      } 
    } 

    $this->game_structure = $rounds;
  }
  
  protected function set_match_path($matches, $current_round, $current_match) {
    // Set the winner path in the previous round
    $previous_round = $current_round-1;
    dpm($matches);
    // if (array_key_exists('round-'. $previous_round, $matches)) {
    //   // Previous Match number.
    //   $pm = $current_match * 2;
    //   $previous_match = $pm - 1;
    //   $matches['round-'. $previous_round]['match-'. $previous_match]['winner'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match . '_contestant-1';
    //   $matches['round-'. $previous_round]['match-'. $pm]['winner'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match . '_contestant-2';
    //   $matches['round-'. $current_round]['match-'. $current_match]['previous-1'] = 'bracket-'. $bracket .'_round-'. $previous_round .'_match-'. $previous_match;
    //   $matches['round-'. $current_round]['match-'. $current_match]['previous-2'] = 'bracket-'. $bracket .'_round-'. $previous_round .'_match-'. $pm;
    //   $matches['round-'. $current_round]['match-'. $current_match]['this_match'] = 'bracket-'. $bracket .'_round-'. $current_round .'_match-'. $current_match;
    // }
    return $matches;
  }
}