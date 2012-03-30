(function($) {
  Drupal.behaviors.highlightPath = {
    attach: function(context, settings) {
      $('div.contestant[class*=entity]', context).hover(function() {

        // Contestant divs declare the entity they represent with a CSS class.
        // Grab that class.
        var highlight = $(this).attr('class').match(/\bentity-.*\b/);

        if (highlight) {
          // Highlight contestants
          $('div.' + highlight).addClass('bracket-highlight');
        }
      },
      function() {
        // Remove the highlight on hover out
        $('.bracket-highlight').removeClass('bracket-highlight');
      });
    }
  };
})(jQuery);