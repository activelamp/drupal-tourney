<?php 
if ( !$contestant->name ) $contestant->name = 'Team ' . $contestant->slot;
?>

<div class="contestant contestant-<?php echo $contestant->slot ?>">
  <?php if ( $seed ): ?>
    <span class="seed"><?php echo $seed ?></span>
  <?php endif;?>
  <?php echo $contestant->name; ?>
</div>
