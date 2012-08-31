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
  <?php if ($has_children && count($node['children'])): ?>
    <div class="children <?php print $children_classes; ?>">
      <?php foreach ($node['children'] as $id => $child): ?>
        <?php print theme('tourney_tournament_tree_node', array('node' => $child, 'plugin' => $plugin)); ?>
      <?php endforeach; ?>
    </div>
  <?php endif; ?>
  <div class="parent">
    <?php if ($round_name): ?>
      <h2 class="round-title"><?php print $round_name; ?></h2>
    <?php endif; ?>
    <?php print theme('tourney_match_render', array('match' => $node, 'plugin' => $plugin, 'children_classes' => $children_classes)); ?>
    <div class="connector to-parent <?php print $node_classes; ?> <?php print $path_classes; ?>">
      <div class="path"></div>
    </div>
  </div>
</div>
<div class="clear"></div>