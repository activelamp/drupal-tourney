<?php
/**
 * @file
 * 
 * Template for Manual Upload tournaments.  Shows the standings table, as well
 * as each round of play.
 *
 * @ingroup tourney_templates
 */
?>

<?php foreach ($rounds as $round): ?>
  <div class="round">
    <h2><?php print $round['title']?></h2>
    <div class="matches">
      <?php foreach ($round['matches'] as $match): ?>
        <?php print theme('tourney_match_render', array('match' => $match, 'plugin' => $plugin)); ?>
      <?php endforeach; ?>
    </div>
  </div>
<?php endforeach; ?>
