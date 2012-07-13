<?php
/**
 * @file tourney-match.tpl.php
 * Theme the wrapper of each match
 *
 * @ingroup tourney_templates
 */
?>
<?php print $match['id']; ?>
<div class="match">
  <?php if ($has_children): ?>
    <div class="connector to-children <?php print $children_classes; ?>"></div>
  <?php endif; ?>
  <?php print $contestants; ?>
</div>