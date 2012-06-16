<?php
$is_bye        = isset($node['bye']) ? $node['bye'] : FALSE;
$is_child      = isset($node['child']);
$is_only_child = isset($node['only']) ? $node['only'] : FALSE;
$has_children  = isset($node['children']);

$has_bye   = FALSE;
$both_byes = TRUE;
if ( $has_children ) {
  foreach ( $node['children'] as &$child ) {
    if ( isset($child['bye']) && $child['bye'] )
      $has_bye   = TRUE;
    else
      $both_byes = FALSE;      
  } 
  unset($child);
}

$bye_secondlevel = array();
if ( $has_children ) {
  foreach ( $node['children'] as $id => &$child ) {
    $child['child'] = $id;
    $child['only']  = $has_bye && !$both_byes;
    $child['bye_firstlevel'] = isset($node['bye_secondlevel']) ? $node['bye_secondlevel'] : array();
    $child['bye_secondlevel'] = &$bye_secondlevel;
    if ( isset($child['children']) ) 
      foreach ( $child['children'] as $grandchild ) 
        $bye_secondlevel[] = ( isset($grandchild['bye']) && $grandchild['bye'] );
  }
  unset($child);
}

$node_classes = array();

if ( $is_bye ) $node_classes[] = 'bye';
if ( isset($node['bye_firstlevel']) && $node['bye_firstlevel'] == array(TRUE, FALSE, FALSE, FALSE) ) {
  if ( $is_bye ) $node_classes[] = 'no-height-change';
}
else {
  if ( !$is_bye && $is_only_child ) $node_classes[] = 'only-child';
}
$node_classes = implode(' ', $node_classes);

$children_classes = array();
if ( $both_byes )     $children_classes[] = 'bye';
$children_classes = implode(' ', $children_classes);
?>

<div class="tree-node <?php echo $node_classes; ?>">
  <?php if ( $has_children ): ?>
    <div class="children <?php echo $children_classes; ?>">
      <?php foreach ( $node['children'] as $id => $child ) {
          echo theme('tourney_tournament_tree_node', array('node' => $child)); 
      } ?>
    </div>
    <div class="connector to-children <?php echo $children_classes; ?>"><div class="path"></div></div>
  <?php endif; ?>
  <div class="parent">
    <?php echo theme('tourney_match', array('match' => $node)); ?>
  </div>
  <div class="connector to-parent<?php echo $is_child ? ' child-' . $node['child'] : ''; ?>">
    <div class="path"></div>
  </div>
</div>