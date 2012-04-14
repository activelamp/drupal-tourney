<?php
$element = $variables['element'];
$children = element_children($element);

$header = array('Team Name');
$rows   = array();

for ( $gn = 0; $gn < $element['#games']; $gn++ ) {
  $header[] = "Game " . ( $gn + 1 );
  if ( $gn < count($children) ) {
    $game = $element[$children[$gn]];
    foreach ( element_children($game) as $user ) {
      if ( !isset($rows[$user]) ) $rows[$user][] = $game[$user]['#title'];
      $rows[$user][] = drupal_render($game[$user]);
    }
  }
  else {
    foreach ( $rows as $id => &$row ) $row[] = ' - ';
  }
}

$header[] = "Results";
foreach ( $rows as $id => &$row ) $row[] = ' - ';

echo theme('table', array(
  'attributes' => array('id' => 'game-list'),
  'header' => $header,
  'rows' => $rows,
));
?>