<?php
$is_bye        = property_exists($node, 'bye') ? $node->bye : FALSE;
$is_child      = property_exists($node, 'child');
$is_only_child = property_exists($node, 'only') ? $node->only : FALSE;
$has_children  = property_exists($node, 'children');

$has_bye   = FALSE;
$both_byes = TRUE;
if ( $has_children ) {
  foreach ( $node->children as $child ) {
    if ( property_exists($child, 'bye') && $child->bye )
      $has_bye   = TRUE;
    else
      $both_byes = FALSE;      
  } 
}

$child_byes = 0;
if ( $has_children ) {
  foreach ( $node->children as $id => $child ) {
    $child->child = $id;
    $child->only  = $has_bye && !$both_byes;
    $child->sibling_byes = &$child_byes;
    if ( property_exists($child, 'bye') && $child->bye ) 
      $child_byes++;
  }
}

$node_classes = array();

if ( $is_bye ) $node_classes[] = 'bye';
if ( property_exists($node, 'sibling_byes') && $node->sibling_byes == 1 ) {
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
      <?php foreach ( $node->children as $id => $child ) {
          echo theme('tourney_tournament_tree_node', array('node' => $child)); 
      } ?>
    </div>
    <div class="connector to-children <?php echo $children_classes; ?>"><div class="path"></div></div>
  <?php endif; ?>
  <div class="parent">
    <?php echo theme('tourney_match', array('match' => $node)); ?>
  </div>
  <div class="connector to-parent<?php echo $is_child ? ' child-' . $node->child : ''; ?>">
    <div class="path"></div>
  </div>
</div>