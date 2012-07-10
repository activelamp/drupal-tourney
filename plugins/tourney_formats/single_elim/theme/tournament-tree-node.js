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
			$children = $this.prev().children().children().filter('.parent');
		
		// Only double connectors can become unbalanced
    if($children.length > 1)
    {
			var $connectTop = $children.find('.path').first(),
				$connectBottom = $children.find('.path').last(),
				$connectParent = $this.find('.connector.to-children .path'),
				
				connectOffset = $connectBottom.offset().top - $connectParent.offset().top;
			
			if(connectOffset)
			{
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