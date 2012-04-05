<?php

/**
 * @file
 * Defines the default rules for tourney module.
 */

/**
 * Implements hook_default_rules_configuration().
 */
function tourney_default_rules_configuration() {
  $rule = rules_reaction_rule();
  $rule->label = 'Tourney game is won';
  $rule->active = TRUE;
  $rule->event('tourney_game_update')
        ->condition('entity_is_of_type', array(
          'entity:select' => 'tourney-game:relation-has-game-tourney-match-reverse:0',
          'type' => 'tourney_match'
        ))
        ->action('tourney_action_get_game_count', array(
          'match:select' => 'tourney-game:relation-has-game-tourney-match-reverse:0'
        ))
        ->action('data_calc', array(
          'input_1:select' => 'tourney-game:relation-has-game-tourney-match-reverse:0:games',
          'op' => '/',
          'input_2:select' => 'number-of-won-games',
          'result:label' => 'Quotient of Total Games / Games won by leader',
          'result:var' => 'games_quotient',
        ))
        ->action('tourney_action_game_is_won', array(
          'match:select' => 'tourney-game:relation-has-game-tourney-match-reverse:0',
          'quotient:select' => 'games-quotient',
        ))
        ->action('drupal_message', array('message:select' => 'action-message'));;

  $configs['rules_tourney_game_is_won'] = $rule;

  return $configs;
}