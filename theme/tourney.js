(function($) {
  Drupal.behaviors.highlightPath = {
    attach: function(context, settings) {
      $('div.contestant[class*=entity]', context).hover(function() {

        // Contestant divs declare the entity they represent with a CSS class.
        // Grab that class.
        var highlight = $(this).attr('class').match(/\bentity-.*\b/);

        if (highlight) {
          // Highlight contestants
          // Highlight one bracket at a time to avoid highlighting loser paths
          // in double elimination type tournaments
          $('div.bracket').each(function() {
            $('div.' + highlight, $(this)).addClass('bracket-highlight')
              .not(':last').closest('div.match-contestant').find('div.flow').addClass('bracket-highlight');
          });
        }
      },
      function() {
        // Remove the highlight on hover out
        $('.bracket-highlight').removeClass('bracket-highlight');
      });
    }
  };
})(jQuery);