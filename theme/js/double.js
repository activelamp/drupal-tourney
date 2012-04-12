(function($) {
  Drupal.behaviors.highlightDouble = {
    attach: function(context, settings) {
      $('div.contestant[class*=entity]', context).hover(function() {

        // Contestant divs declare the entity they represent with a CSS class.
        // Grab that class.
        var highlight = $(this).attr('class').match(/\bentity-.*\b/);

        if (highlight) {
          
          // For each bracket, highlight the team's appearances and flow lines.
          // If this is the last time they appear in the bracket, they probably
          // lost. Don't highlight the flow lines for that.
          $('div.bracket:not(div.bracket-wrapper)').each(function() {
            $('div.' + highlight, $(this)).addClass('bracket-highlight')
              .not(':last').closest('div.match-contestant').find('div.flow').addClass('bracket-highlight');
            if ($(this).hasClass('bracket-champion')) {
              
            }
          });
          // However, when we check the championship bracket, go back and
          // highlight the last appearance in the last bracket they were in.
          // That's the appearance that took them to the chamionship.
          if ($('div.bracket-champion').has('div.' + highlight).length) {
            $('div.bracket-top, div.bracket-bottom').find('div.' + highlight).last()
              .closest('div.match-contestant').find('div.flow').addClass('bracket-highlight');
          }
        }
      },
      function() {
        // Remove the highlight on hover out
        $('.bracket-highlight').removeClass('bracket-highlight');
      });
    }
  };
})(jQuery);