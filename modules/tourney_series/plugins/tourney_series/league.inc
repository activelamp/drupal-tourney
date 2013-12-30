<?php

/**
 * @file
 * Tourney series plugin that has no logic.
 */

/**
 * Generic tournament series.
 */

if (module_exists('tourney_league')) {
  $plugin = array(
    'title' => t('League series'),
    'machine name' => 'league',
    'description' => t('Group tournaments together that are in the same league.'),
    'weight' => 0,
    // When we are ready to add base functionality, create the class below.
    'controller' => 'LeagueTourneySeriesController',
  );
}

class LeagueTourneySeriesController extends TourneySeriesController {

  public function optionsForm(&$form_state) {
    $form = parent::optionsForm($form_state);

    return $form;
  }

  /**
   * Ensure that all the tournaments are in the same league.
   *
   * @param $form
   * @param $form_state
   */
  public static function optionsFormValidate(&$form, &$form_state) {
    $tournaments = $form_state['values']['tournaments'][LANGUAGE_NONE];
    $league_id = $message = NULL;
    foreach ($tournaments as $info) {
      if (empty($info['target_id'])) continue;
      $tournament = tourney_tournament_load($info['target_id']);
      if (empty($tournament->field_league_reference)) {
        $message = t('All tournaments must have a league defined with League Series');
      }
      else if (!is_null($league_id) && $tournament->field_league_reference[LANGUAGE_NONE][0] != $league_id) {
        $message = t('All tournaments must have the same league defined with League Series');
      }
      if ($message) {
        form_set_error('format', $message);
      }
    }
  }

}