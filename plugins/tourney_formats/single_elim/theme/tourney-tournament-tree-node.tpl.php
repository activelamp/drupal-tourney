<?php
/**
 * @file tourney-tournament-tree-node.tpl.php
 * 
 * Creates the table based layout of each match and its children.  This template
 * is recursive and will create each match and it's children mark-up in the
 * children div of each iteration.
 *
 * @ingroup tourney_templates
 */
?>

<div class="tree-node <?php print $node_classes; ?>">
  <?php if ($has_children): ?>
    <div class="children <?php print $children_classes; ?>">
      <?php foreach ($node['children'] as $id => $child): ?>
        <?php print theme('tourney_tournament_tree_node', array('node' => $child, 'plugin' => $plugin)); ?>
      <?php endforeach; ?>
    </div>
    <div class="connector to-children <?php print $children_classes; ?>"><div class="path"></div></div>
  <?php endif; ?>
  <div class="parent">
    <?php print theme('tourney_match_render', array('match' => $node, 'plugin' => $plugin)); ?>
  </div>
  <div class="connector to-parent<?php print $is_child ? ' child-' . $node['child'] : ''; ?>">
    <div class="path"></div>
  </div>
</div>