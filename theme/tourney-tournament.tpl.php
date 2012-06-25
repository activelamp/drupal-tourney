<?php
/**
 * @file tourney-tournament.tpl.php
 * Theme the wrapper of a tournament
 *
 * - $structure: The structure of the tournament matches.
 * - $matches: The rendered matches from the plugin.
 *
 * @ingroup tourney_templates
 */
?>

<div class="<?php print $classes; ?>">
  <?php print $matches; ?>
</div>