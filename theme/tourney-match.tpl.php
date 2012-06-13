<?php
  $null_contestant = array('name' => NULL, 'id' => NULL);
  $seeds = $match->getSeeds();
  $contestants = $match->getContestants()
    + array(1 => (object) $null_contestant, 2 => (object) $null_contestant);
  ksort($contestants);
  foreach ( $contestants as $slot => $contestant ) {
    $contestant->slot = $slot;
  }
?>

<div class="match">
  <?php foreach ( $contestants as $slot => $contestant ) {
    echo theme('tourney_contestant', array('contestant' => $contestant, 'seed' => isset($seeds[$slot]) ? $seeds[$slot] : NULL));
  } ?>
</div>