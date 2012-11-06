<?php

/*
 * @file
 */

/**
 * Implements hook_views_data_alter().
 */
function tourney_views_data_alter(&$data) {
  $data['tourney_tournament']['edit_link'] = array(
    'field' => array(
      'title' => t('Edit Link'),
      'help' => t('Provide a simple link to edit the tournament.'),
      'handler' => 'tourney_handler_field_entity_link_edit',
    )
  );

  $data['tourney_tournament']['match_view_link'] = array(
    'field' => array(
      'title' => t('Match View Link'),
      'help' => t('Provide a link to the match view.'),
      'handler' => 'tourney_handler_field_match_view_link',
    )
  );

  // Tournery Match additions

  $data['tourney_match']['edit_link'] = array(
    'field' => array(
      'title' => t('Edit Link'),
      'help' => t('Provide a simple link to edit the match.'),
      'handler' => 'tourney_handler_field_entity_link_edit',
    )
  );

  $data['tourney_match']['game_view_link'] = array(
    'field' => array(
      'title' => t('Game View Link'),
      'help' => t('Provide a link to list the games in a given match.'),
      'handler' => 'tourney_handler_field_game_view_link',
    )
  );

  $data['tourney_match']['match_tree'] = array(
    'title' => t('Match Tree'),
    'help' => t('Match ID with depth'),
    'field' => array(
      'real field' => 'id',
      'handler' => 'views_handler_field',
    ),
    'argument' => array(
      'real field' => 'id',
      'handler' => 'tourney_handler_match_tree',
    ),
  );
  $data['tourney_match']['match_tree_depth'] = array(
    'title' => t('Match Tree Depth'),
    'help' => t('Match ID custom depth filter'),
    'field' => array(
      'real field' => 'id',
      'handler' => 'views_handler_field',
    ),
    'argument' => array(
      'real field' => 'id',
      'handler' => 'views_handler_argument',
    ),
  );
  // Define the field for the gameIds on the match object
  $data['tourney_match']['game_ids'] = array(
    'title' => t('Game Ids'),
    'help' => t('An array of game ids loaded on the match object'),
    'field' => array(
      'handler' => 'tourney_handler_field_object_property',
    ),
  );
  $data['tourney_match']['tournament_id'] = array(
    'title' => t('Tournament Id'),
    'help' => t('The tournament id loaded on the match object'),
    'field' => array(
      'handler' => 'tourney_handler_field_object_property',
    ),
  );
  
  // Tourney Game additions
  $data['tourney_game']['edit_game'] = array(
    'field' => array(
      'title' => t('Edit link'),
      'help' => t('Provide a simple link to edit the game.'),
      'handler' => 'tourney_handler_field_entity_link_edit',
    ),
  );
  
  // generic footer/header links
  $data['views']['parent_match'] = array(
    'title' => t('Link to Parent Match'),
    'help' => t('Provide a link to the parent match.'),
    'area' => array(
      'handler' => 'views_handler_parent_match_link',
    ),
  );
  $data['views']['parent_tourney'] = array(
    'title' => t('Link to Parent Tournament'),
    'help' => t('Provide a link to the parent tourney.'),
    'area' => array(
      'handler' => 'views_handler_parent_tourney_link',
    ),
  );


  $data['tourney_rank']['rank_change'] = array(
    'title' => t('Rank Change'),
    'help' => t('Contestant rank change.'),
    'field' => array(
      'handler' => 'tourney_handler_field_rank',
    ),
  );

  $data['tourney_rank']['table']['group'] = t('Tourney Rank');
  $data['tourney_rank']['table']['join'] = array(
    'users' => array(
      'left_field' => 'uid',
      'field' => 'entity_id',
      'extra' => array(
        array('field' => 'entity_type', 'value' => 'user'),
      ),
    ),
    'node' => array(
      'left_field' => 'nid',
      'field' => 'entity_id',
      'extra' => array(
        array('field' => 'entity_type', 'value' => 'node'),
      ),
    ),
  );

  $data['tourney_rank']['tournament'] = array(
    'title' => t('Tournament ID'),
    'help' => t('Tournament to filter rank rows.'),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_numeric',
    ),
  );

  $data['tourney_rank']['rank'] = array(
    'title' => t('Contestant Rank'),
    'help' => t('Rank contestant has achieved in the tournament.'),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tourney_rank']['wins'] = array(
    'title' => t('Contestant Wins'),
    'help' => t('Number of contestant Wins.'),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tourney_rank']['losses'] = array(
    'title' => t('Contestant Losses'),
    'help' => t('Number of contestant Losses.'),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
}

function tourney_views_handlers() {
  return array(

  );
}