<?php

/**
 * @file
 * Default views for tourney module.
 */

/**
 * Implements hook_views_default_views().
 */
function tourney_views_default_views() {
  $views = array();

  $view = new view;
  $view->name = 'games_in_match';
  $view->description = 'Provides a local tab for all games in match';
  $view->tag = 'default';
  $view->base_table = 'tourney_match';
  $view->human_name = 'Games in match';
  $view->core = 7;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Master */
  $handler = $view->new_display('default', 'Master', 'default');
  $handler->display->display_options['access']['type'] = 'none';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['query']['options']['query_comment'] = FALSE;
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'full';
  $handler->display->display_options['style_plugin'] = 'table';
  $handler->display->display_options['style_options']['columns'] = array(
    'id' => 'id',
    'title' => 'title',
  );
  $handler->display->display_options['style_options']['default'] = 'id';
  $handler->display->display_options['style_options']['info'] = array(
    'id' => array(
      'sortable' => 0,
      'default_sort_order' => 'asc',
      'align' => '',
      'separator' => '',
      'empty_column' => 0,
    ),
    'title' => array(
      'sortable' => 0,
      'default_sort_order' => 'asc',
      'align' => '',
      'separator' => '',
      'empty_column' => 0,
    ),
  );
  $handler->display->display_options['style_options']['override'] = 1;
  $handler->display->display_options['style_options']['sticky'] = 0;
  $handler->display->display_options['style_options']['empty_table'] = 0;
  /* Relationship: Tourney_match: Relation: has game (tourney_match -&gt; tourney_game) */
  $handler->display->display_options['relationships']['relation_has_game_tourney_game']['id'] = 'relation_has_game_tourney_game';
  $handler->display->display_options['relationships']['relation_has_game_tourney_game']['table'] = 'tourney_match';
  $handler->display->display_options['relationships']['relation_has_game_tourney_game']['field'] = 'relation_has_game_tourney_game';
  $handler->display->display_options['relationships']['relation_has_game_tourney_game']['required'] = 0;
  $handler->display->display_options['relationships']['relation_has_game_tourney_game']['r_index'] = '-1';
  $handler->display->display_options['relationships']['relation_has_game_tourney_game']['entity_deduplication_left'] = 0;
  $handler->display->display_options['relationships']['relation_has_game_tourney_game']['entity_deduplication_right'] = 0;
  /* Field: Tourney_game: Tourney_game ID */
  $handler->display->display_options['fields']['id']['id'] = 'id';
  $handler->display->display_options['fields']['id']['table'] = 'tourney_game';
  $handler->display->display_options['fields']['id']['field'] = 'id';
  $handler->display->display_options['fields']['id']['relationship'] = 'relation_has_game_tourney_game';
  $handler->display->display_options['fields']['id']['label'] = 'Game ID';
  $handler->display->display_options['fields']['id']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['id']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['id']['alter']['absolute'] = 0;
  $handler->display->display_options['fields']['id']['alter']['external'] = 0;
  $handler->display->display_options['fields']['id']['alter']['replace_spaces'] = 0;
  $handler->display->display_options['fields']['id']['alter']['trim_whitespace'] = 0;
  $handler->display->display_options['fields']['id']['alter']['nl2br'] = 0;
  $handler->display->display_options['fields']['id']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['id']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['id']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['id']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['id']['alter']['html'] = 0;
  $handler->display->display_options['fields']['id']['element_label_colon'] = 1;
  $handler->display->display_options['fields']['id']['element_default_classes'] = 1;
  $handler->display->display_options['fields']['id']['hide_empty'] = 0;
  $handler->display->display_options['fields']['id']['empty_zero'] = 0;
  $handler->display->display_options['fields']['id']['hide_alter_empty'] = 1;
  $handler->display->display_options['fields']['id']['format_plural'] = 0;
  /* Field: Tourney_game: Label */
  $handler->display->display_options['fields']['title']['id'] = 'title';
  $handler->display->display_options['fields']['title']['table'] = 'tourney_game';
  $handler->display->display_options['fields']['title']['field'] = 'title';
  $handler->display->display_options['fields']['title']['relationship'] = 'relation_has_game_tourney_game';
  $handler->display->display_options['fields']['title']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['title']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['title']['alter']['absolute'] = 0;
  $handler->display->display_options['fields']['title']['alter']['external'] = 0;
  $handler->display->display_options['fields']['title']['alter']['replace_spaces'] = 0;
  $handler->display->display_options['fields']['title']['alter']['trim_whitespace'] = 0;
  $handler->display->display_options['fields']['title']['alter']['nl2br'] = 0;
  $handler->display->display_options['fields']['title']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['title']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['title']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['title']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['title']['alter']['html'] = 0;
  $handler->display->display_options['fields']['title']['element_label_colon'] = 1;
  $handler->display->display_options['fields']['title']['element_default_classes'] = 1;
  $handler->display->display_options['fields']['title']['hide_empty'] = 0;
  $handler->display->display_options['fields']['title']['empty_zero'] = 0;
  $handler->display->display_options['fields']['title']['hide_alter_empty'] = 1;
  /* Field: Tourney_game: Edit link */
  $handler->display->display_options['fields']['edit_game']['id'] = 'edit_game';
  $handler->display->display_options['fields']['edit_game']['table'] = 'tourney_game';
  $handler->display->display_options['fields']['edit_game']['field'] = 'edit_game';
  $handler->display->display_options['fields']['edit_game']['relationship'] = 'relation_has_game_tourney_game';
  $handler->display->display_options['fields']['edit_game']['label'] = 'Edit Game';
  $handler->display->display_options['fields']['edit_game']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['absolute'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['replace_spaces'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['trim_whitespace'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['nl2br'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['edit_game']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['edit_game']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['edit_game']['alter']['html'] = 0;
  $handler->display->display_options['fields']['edit_game']['element_label_colon'] = 0;
  $handler->display->display_options['fields']['edit_game']['element_default_classes'] = 1;
  $handler->display->display_options['fields']['edit_game']['hide_empty'] = 0;
  $handler->display->display_options['fields']['edit_game']['empty_zero'] = 0;
  $handler->display->display_options['fields']['edit_game']['hide_alter_empty'] = 1;
  /* Contextual filter: Tourney_match: Tourney_match ID */
  $handler->display->display_options['arguments']['id']['id'] = 'id';
  $handler->display->display_options['arguments']['id']['table'] = 'tourney_match';
  $handler->display->display_options['arguments']['id']['field'] = 'id';
  $handler->display->display_options['arguments']['id']['default_action'] = 'empty';
  $handler->display->display_options['arguments']['id']['default_argument_type'] = 'fixed';
  $handler->display->display_options['arguments']['id']['default_argument_skip_url'] = 0;
  $handler->display->display_options['arguments']['id']['summary']['number_of_records'] = '0';
  $handler->display->display_options['arguments']['id']['summary']['format'] = 'default_summary';
  $handler->display->display_options['arguments']['id']['summary_options']['items_per_page'] = '25';
  $handler->display->display_options['arguments']['id']['break_phrase'] = 0;
  $handler->display->display_options['arguments']['id']['not'] = 0;

  /* Display: Page */
  $handler = $view->new_display('page', 'Page', 'page_1');
  $handler->display->display_options['path'] = 'tourney/match/%/games';
  $handler->display->display_options['menu']['type'] = 'tab';
  $handler->display->display_options['menu']['title'] = 'Games';
  $handler->display->display_options['menu']['weight'] = '0';
  $handler->display->display_options['menu']['context'] = 0;
  $handler->display->display_options['tab_options']['weight'] = '0';
  $views['games_in_match'] = $view;

  return $views;
}
