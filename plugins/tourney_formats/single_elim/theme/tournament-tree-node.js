jQuery(function($)
{
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
	$('.parent').each(function()
	{
		var $this = $(this),
			// #TOURNEYDOM
      // Finding the child parent element of a parent.
			$children = $this.prev().children().children().children().filter('.parent');
	
		// Only double connectors can become unbalanced
		console.log($children.length);
    if($children.length > 1)
    {
      
			var $connectTop = $children.find('.to-parent .path').first(),
				$connectBottom = $children.find('.to-parent .path').last(),
				$connectParent = $this.find('.connector.to-children .path'),
				
				connectOffset = $connectBottom.offset().top - $connectParent.offset().top;

			if(connectOffset) {

				$connectTop.css({
					height: $connectTop.height() - connectOffset,
					bottom: connectOffset
				});
				
				$connectBottom.css({
					height: $connectBottom.height() + connectOffset,
					top: -connectOffset
				});
			}
    }
	});
});