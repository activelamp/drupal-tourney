(function($) {
  Drupal.behaviors.setWidth = {
    attach: function(context, settings) {
      $(".tourney", context).each(function(){
        $rounds = $(".tourney-single .round, .bracket-top .round, .bracket-champion .round", context);
        rw = $($rounds[0]).css('width');
        if ( !rw ) return;
        $('.tourney-inner', this).css('width', parseInt(rw)*$rounds.length+'px');
      });
    }
  };
})(jQuery);