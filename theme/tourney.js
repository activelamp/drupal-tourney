(function($) {
  Drupal.behaviors.tourneySetWidth = {
    attach: function(context, settings) {
      $(".tourney", context).each(function(){
        $rounds = $(".tourney-single .round, .bracket-top .round, .bracket-champion .round:not(:empty)", context);
        rw = $($rounds[0]).css('width');
        if ( !rw ) return;
        $('.tourney-inner', this).css('width', (parseInt(rw)+parseInt(rw)*$rounds.length)+'px');
      });
    }
  };
  Drupal.behaviors.tourneyDisableWin = {
    attach: function(context, settings) {
      $(".win-button", context).click(function() {
        $(".win-button", context).each(function() {
          $(this).hide();
          $('<div class="ajax-progress"><span class="throbber"></span></div>').insertAfter(this);
        })
      });
    }
  };
})(jQuery);