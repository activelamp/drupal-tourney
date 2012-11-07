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
       ->action('drupal_message', array('message:select' => 'action-message'));

  $configs['rules_tourney_game_is_won'] = $rule;

  $rule = rules_reaction_rule();
  $rule->label = 'Set round robin paths';
  $rule->active = TRUE;
  $rule->event('tourney_match_update')
       ->condition('data_is', array(
         'data:select' => 'tourney-match:relation-has-match-tourney-tournament-reverse:0:format',
         'value' => 'RoundRobinController'
       ))
       ->condition('data_is_empty', array(
         'data:select' => 'tourney-match:relation-has-game-tourney-game:0:winner',
       ))
       ->condition('tourney_condition_match_has_contestants', array(
         'match:select' => 'tourney-match',
       ))
       ->action('tourney_action_set_all_matches', array(
         'match:select' => 'tourney-match',
       ))
       ->action('drupal_message', array('message:select' => 'action-message'));

  $configs['rules_tourney_set_all_matches'] = $rule;

  $rule = rules_reaction_rule();
  $rule->label = 'Set regular season paths';
  $rule->active = TRUE;
  $rule->event('tourney_match_update')
       ->condition('data_is', array(
         'data:select' => 'tourney-match:relation-has-match-tourney-tournament-reverse:0:format',
         'value' => 'RegularSeasonController'
       ))
       ->condition('data_is_empty', array(
         'data:select' => 'tourney-match:relation-has-game-tourney-game:0:winner',
       ))
       ->condition('tourney_condition_match_has_contestants', array(
         'match:select' => 'tourney-match',
       ))
       ->action('tourney_action_set_regularseason_matches', array(
         'match:select' => 'tourney-match',
       ))
       ->action('drupal_message', array('message:select' => 'action-message'));

  $configs['rules_tourney_set_regularseason_matches'] = $rule;

  $rule = rules_reaction_rule();
  $rule->label = 'Tourney bye is created';
  $rule->active = TRUE;
  $rule->event('tourney_match_update')
       ->condition('data_is', array(
         'data:select' => 'tourney-match:games',
         'value' => '0'
       ))
       ->action('tourney_action_game_is_won', array(
         'match:select' => 'tourney-match',
         'quotient' => '1',
       ))
       ->action('drupal_message', array('message:select' => 'action-message'));

  $configs['rules_tourney_bye_is_created'] = $rule;
  
  // $rule = rules_reaction_rule();
  // $rule->label = 'Tourney Champion Match Is Won2';
  // $rule->active = TRUE;
  // $rule->event('tourney_match_update')
  //      ->condition('text_matches', array(
  //        'text:select' => 'tourney-match:matchinfo-bracket', 
  //        'match' => 'champion',
  //      ))
  //      ->condition('text_matches', array(
  //        'text:select' => 'tourney-match:matchinfo-round', 
  //        'match' => '1',
  //      ))
  //      ->condition('data_is', array(
  //        'data:select' => 'tourney-match:winner', 
  //        'value:select' => 'tourney-match:contestant-1',
  //      ))
  //      ->action('variable_add', array(
  //        'type' => 'integer',
  //        'value:select' => 'tourney-match:id',
  //        'value:settings:num_offset:value' => '1',
  //        'result:label' => 'Next Match ID',
  //        'result:var' => 'next_match_id',
  //      ));
  //
  // $configs['rules_tourney_champion_match2'] = $rule;
  
  // Using rules_import below to import default rule. Rules docs aren't very good
  // in terms of writting this using rules_reaction_rule().
  $rule = rules_import('{ "rules_tourney_champion_match_is_won" : {
      "LABEL" : "Tourney Champion Match Is Won",
      "PLUGIN" : "reaction rule",
      "REQUIRES" : [ "rules", "tourney" ],
      "ON" : [ "tourney_match_update" ],
      "IF" : [
        { "text_matches" : { "text" : [ "tourney-match:matchinfo-bracket" ], "match" : "champion" } },
        { "text_matches" : { "text" : [ "tourney-match:matchinfo-round" ], "match" : "1" } },
        { "data_is" : {
            "data" : [ "tourney-match:winner" ],
            "value" : [ "tourney-match:contestant-1" ]
          }
        }
      ],
      "DO" : [
        { "variable_add" : {
            "USING" : {
              "type" : "integer",
              "value" : { "select" : "tourney-match:id", "num_offset" : { "value" : "1" } }
            },
            "PROVIDE" : { "variable_added" : { "next_match_id" : "Next Match ID" } }
          }
        },
        { "entity_fetch" : {
            "USING" : { "type" : "tourney_match", "id" : [ "next-match-id" ] },
            "PROVIDE" : { "entity_fetched" : { "next_match" : "Next Match" } }
          }
        },
        { "data_set" : { "data" : [ "next-match:status" ], "value" : "0" } },
        { "drupal_message" : { "message" : "Match [tourney-match:id] was won, and [tourney-match:contestant_1] remains undefeated. Unpublishing match [match:id]." } }
      ]
    }
  }');
  $configs[$rule->name] = $rule;

  return $configs;
}