<?php
/**
 * @file tourney-contestant.tpl.php
 * Theme the wrapper for each contestant
 *
 * @ingroup tourney_templates
 */
?>

<div class="contestant contestant-<?php print $contestant->slot ?>">
  <?php if ($seed): ?>
    <span class="seed"><?php print $seed ?></span>
  <?php endif;?>
  <span title="<?php print $contestant->name; ?>"><?php print $contestant->label; ?></span>
</div>
