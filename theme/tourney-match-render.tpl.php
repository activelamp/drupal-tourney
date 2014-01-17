<?php
/**
 * @file tourney-match.tpl.php
 * Theme the wrapper of each match
 *
 * @ingroup tourney_templates
 */
?>
<a href="<?= drupal_get_path_alias($match->uri); ?>">
	<div class="match <?php print $match_classes; ?>">
	  <?php if ($has_children): ?>
	    <div class="connector to-children"></div>
	  <?php endif; ?>
	  <?php print $contestants; ?>
	</div>
</a>