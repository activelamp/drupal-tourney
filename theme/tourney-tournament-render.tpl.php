<?php
/**
 * @file tourney-tournament.tpl.php
 * Theme the wrapper of a tournament
 *
 * - $structure: The structure of the tournament matches.
 * - $matches: The rendered matches from the plugin.
 * - $header: The tourney header.
 * - $footer: The tourney footer.
 *
 * @ingroup tourney_templates
 */
?>

<?php if ($header): ?>
  <div class="tourney-header">
    <?php print $header; ?>
  </div>
<?php endif; ?>

<div class="tourney-tournament <?php print $classes; ?>">
  <?php print $matches; ?>
</div>

<?php if ($footer): ?>
  <div class="tourney-footer">
    <?php print $footer; ?>
  </div>
<?php endif; ?>