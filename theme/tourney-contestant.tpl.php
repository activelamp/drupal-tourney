<?php
/**
 * @file tourney-contestant.tpl.php
 * Theme the wrapper for each contestant
 *
 * @ingroup tourney_templates
 */
?>
<div class="contestant <?php print $classes ?>">
  <?php if ($seed): ?>
    <span class="seed"><?php print $seed ?></span>
  <?php endif;?>
  <span title="<?php print $name; ?>"><?php print $label; ?></span>
  <span class="wins"><?php print isset($wins) ? $wins : '0'; ?></span>
</div>
