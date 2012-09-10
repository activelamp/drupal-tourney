<?php
/**
 * @file tourney-match.tpl.php
 * Theme the wrapper of each match
 *
 * @ingroup tourney_templates
 */
$letters = array('', 
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'AA', 'AB', 'AC', 'AD', 'AE', 'AF', 'AG', 'AH', 'AI', 'AJ', 'AK', 'AL', 'AM', 'AN', 'AO', 'AP', 'AQ', 'AR', 'AS', 'AT', 'AU', 'AV', 'AW', 'AX', 'AY', 'AZ',
  'BA', 'BB', 'BC', 'BD', 'BE', 'BF', 'BG', 'BH', 'BI', 'BJ', 'BK', 'BL', 'BM', 'BN', 'BO', 'BP', 'BQ', 'BR', 'BS', 'BT', 'BU', 'BV', 'BW', 'BX', 'BY', 'BZ',
  );
?>
<div class="match <?php print $match_classes; ?>">
  <?php if ($has_children): ?>
    <div class="connector to-children"></div>
  <?php endif; ?>
  <?php 
  foreach ( array(1 => 'winner', 2 => 'loser') as $k => $v ) {
    print "<div class=\"contestant tourney-contestant contestant-$k\">";
    print (isset($match['previousMatches'][$k]) ? $letters[$match['previousMatches'][$k]] : '@') . ' - ';
    print $letters[$match['id']] . ' - ';
    print (isset($match['nextMatch'][$v]['id']) ? $letters[$match['nextMatch'][$v]['id']] : '@');
    print (isset($match['nextMatch'][$v]['slot']) ? ' - ' . $match['nextMatch'][$v]['slot'] : '@');
    print isset($match['seeds']) && isset($match['seeds'][$k]) ? ' - ' . $match['seeds'][$k] : '';
    print "</div>";
  } 
  ?>
</div>