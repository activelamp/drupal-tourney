<?php

/**
 * @file
 * Default theme implementation for tourney matches.
 *
 * Available variables:
 * - $content: An array of match items. Use render($content) to print them all.
 */
?>

<div class="tourney-match">
  
  <?php print render($title_prefix); ?>
  <?php if (!$page): ?>
    <h2<?php print $title_attributes; ?>><a href="/<?php print $tourney_match->uri; ?>"><?php print $title; ?></a></h2>
  <?php endif; ?>
  <?php print render($title_suffix); ?>
  
  <div class="content">
    <?php print render($content); ?>
  </div>
  
</div>