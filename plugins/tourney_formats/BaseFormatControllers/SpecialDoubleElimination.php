<?php

/**
 * @file
 * Double elimination controller, new system.
 */

/**
 * A class defining how matches are created, and rendered for this style
 * tournament.
 */
class SpecialDoubleEliminationController extends SingleEliminationController {
  /**
   * Theme implementations specific to this plugin.
   */
  public static function theme($existing, $type, $theme, $path) {
    $parent_info = TourneyController::getPluginInfo(get_parent_class($this));
    return parent::theme($existing, $type, $theme, $parent_info['path']);
  }
  
  public function buildBrackets() {
    parent::buildBrackets();
    $this->data['brackets']['loser'] = $this->buildBracket(array('id' => 'loser'));
    $this->data['brackets']['champion'] = $this->buildBracket(array('id' => 'champion'));
  }
  
  public function buildMatches() {
    parent::buildMatches();
    $this->buildBottomMatches();
    $this->buildChampionMatches();
  }
  
  public function buildBottomMatches() {
    $match = &drupal_static('match', 0);
    $round = &drupal_static('round', 0);
    
    // Rounds is a certain number, 2, 4, 6, based on the contestants
    $num_rounds = (log($this->slots, 2) - 1) * 2;
    foreach (range(1, $num_rounds) as $round_num) {
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round_num, 'bracket' => 'loser'));
        
      // Bring the round number down to a unique number per group of two
      $round_group = ceil($round_num / 2);
      
      // Matches is a certain number based on the round number and slots
      // The pattern is powers of two, counting down: 8 8 4 4 2 2 1 1
      $num_matches = $this->slots / pow(2, $round_group + 1);
      foreach (range(1, $num_matches) as $roundMatch) {
        $this->data['matches'][++$match] = 
          $this->buildMatch(array(
            'id' => $match,
            'round' => $round_num,
            'roundMatch' => (int) $roundMatch,
            'bracket' => 'loser',
        ));
      }
    }
  }
  
  public function buildChampionMatches() {
    $match = &drupal_static('match', 0);
    $round = &drupal_static('round', 0);
    
    foreach (array(1, 2) as $round_num) {
      $this->data['rounds'][++$round] = 
        $this->buildRound(array('id' => $round_num, 'bracket' => 'champion'));
      
      $this->data['matches'][++$match] = 
        $this->buildMatch(array(
          'id' => $match,
          'round' => $round_num,
          'roundMatch' => 1,
          'bracket' => 'champion',
      ));
    }
  }
  
  public function render() {
    return parent::render();
  }
}
