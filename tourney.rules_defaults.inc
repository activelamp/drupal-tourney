<?php

/**
 * @file
 * Defines the default rules for tourney module.
 */

/**
 * Implements hook_default_rules_configuration().
 */
function tourney_default_rules_configuration() {
  $rule = rules_import('{ "rules_tourney_set_all_matches" : {
      "LABEL" : "Set round robin paths",
      "PLUGIN" : "reaction rule",
      "REQUIRES" : [ "rules", "tourney" ],
      "ON" : [ "tourney_match_update" ],
      "IF" : [
        { "data_is_empty" : { "data" : [ "tourney-match:relation-has-game-tourney-game:0:winner" ] } },
        { "tourney_condition_match_has_contestants" : { "match" : [ "tourney-match" ] } },
        { "OR" : [
            { "data_is" : {
                "data" : [ "tourney-match:relation-has-match-tourney-tournament-reverse:0:format" ],
                "value" : "ManualUploadController"
              }
            },
            { "data_is" : {
                "data" : [ "tourney-match:relation-has-match-tourney-tournament-reverse:0:format" ],
                "value" : "RoundRobinController"
              }
            }
          ]
        }
      ],
      "DO" : [
        { "tourney_action_set_all_matches" : {
            "USING" : { "match" : [ "tourney-match" ] },
            "PROVIDE" : { "action_message" : { "action_message" : "Action successful message" } }
          }
        },
        { "drupal_message" : { "message" : [ "action-message" ] } }
      ]
    }
  }');
  $configs[$rule->name] = $rule;
    
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