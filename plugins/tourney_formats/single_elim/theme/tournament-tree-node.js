(function($) {

/**
 * Preselectors
 */
var $tournament = $('.tourney-tournament-tree'),
  $teams = {},
  $activeTeam = $();

/**
 * This rebalances all bracket children connectors, so that they line up correctly
 * with the parent connector. The CSS cannot get this "quite" correct alone.
 */
Drupal.behaviors.tourneyFixConnectors = { attach: function(context, settings) {
  $('.tree-node', context).each(function() {
    var $this = $(this);
    var $children = $this.children('.children').children('.tree-node');
    var $parent   = $this.children('.parent');

    if ($children.length == 0) return;

    var $connectors = $('.connector .path', $children.children('.parent'));

    var $connectParent = $('.connector', $parent);
    var $connectBottom = $connectors.last();
    var $connectTop    = null;
    if ($connectors.length > 1) 
      $connectTop      = $connectors.first(); 

    var connectTarget  = $connectParent.offset().top + $connectParent.height();
    var connectOffset  = $connectBottom.offset().top - connectTarget;

    if (connectOffset <= 0) return;    

    if (connectOffset) {
      $connectBottom.css({
        height: $connectBottom.height() + connectOffset,
        top: -connectOffset
      });
      if ($connectTop)
        $connectTop.css('height', $connectTop.height() - connectOffset);
    }
  });
} }

Drupal.behaviors.tourneyFixHeight = { attach: function(context, settings) {
  var $tree   = $('.tourney-tournament-tree', context);
  var current = $tree.offset().top + $tree.height();
  var $match  = $('.tree-node', $tree).last();
  var target  = $match.offset().top + $match.height() + 50;
  $tree.css('padding-bottom', target-current);
} }

Drupal.behaviors.tourneyHighlight = { attach: function(context, settings) {
  $('.contestant', context).hover(function() {
    var eid = '.' + $(this).attr('class').split(' ').reverse()[0];
    $(eid).addClass('tourney-highlight');
    $('.contestant' + eid).closest('.match').addClass('tourney-highlight');
    $('.connector' + eid).each(function(){
      $parent = $(this).closest('.parent').closest('.children').closest('.tree-node');
      if ($(this).hasClass('winner-bottom')) $('.connector.to-children', $(this).parent()).addClass('winner-bottom');
      $('.connector.to-children', $parent.children('.parent')).addClass('tourney-highlight');
    });
  }, function() {
    $('.tourney-highlight').removeClass('tourney-highlight');
  });
} }

})(jQuery)