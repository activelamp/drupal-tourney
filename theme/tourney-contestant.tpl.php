<?php
/**
 * @file tourney-contestant.tpl.php
 * Theme the wrapper for each contestant
 *
 * @ingroup tourney_templates
 */
?>
<?php
if($contestant->type == 'team') {
  $contestant->name = $contestant->title;
  $contestant->label = $contestant->title;
}
?>
<div class="contestant <?php print $classes ?>">
  <?php if ($seed): ?>
    <span class="seed"><?php print $seed ?></span>
  <?php endif;?>
  <span title="<?php print $contestant->name; ?>"><?php print $contestant->label; ?></span>
</div>
