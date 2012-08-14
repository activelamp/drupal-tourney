<?php
/**
 * @file tourney-match.tpl.php
 * Theme the wrapper of each match
 *
 * @ingroup tourney_templates
 */
?>
<div class="match">
  <?php if ($has_children): ?>
    <div class="connector to-children <?php print $children_classes; ?>"></div>
  <?php endif; ?>
  <? /* <div class="contestant tourney-contestant contestant-1"><?php print $match['previousMatches'][1] . ' - ' . $match['id'] . ' - ' . $match['nextMatch']['winner']['id']?></div>
  <div class="contestant tourney-contestant contestant-2"><?php print $match['previousMatches'][2] . ' - ' . $match['id'] . ' - ' . $match['nextMatch']['loser']['id']?></div> */ ?>
  <?php print $contestants; ?>
</div>