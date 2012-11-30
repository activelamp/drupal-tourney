<?php

/**
 * @file
 * Default theme implementation for tourney matches.
 *
 * Available variables:
 * - $title: the (sanitized) title of the match.
 * - $content: An array of match items. Use render($content) to print them all.
 * - $title_prefix (array): An array containing additional output populated by
 *   modules, intended to be displayed in front of the main title tag that
 *   appears in the template.
 * - $title_suffix (array): An array containing additional output populated by
 *   modules, intended to be displayed after the main title tag that appears in
 *   the template.
 * 
 * Other variables:
 * - $tourney_match: Full match object. Contains data that may not be safe. 
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