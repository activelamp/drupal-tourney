<?php
drupal_add_css(drupal_get_path('module', 'tourney') . '/theme/tourney.css');
?>

<div class="tournament-bracket tournament-tree">
  <?php echo theme('tourney_tournament_tree_node', array('node' => $structure['tree'])); ?>
</div>