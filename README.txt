CONTENTS OF THIS FILE
---------------------

 * Introduction
 * Other Solutions
 * Installation
 * Steps to creating a tournament
 * Technical Details


INTRODUCTION
------------

Current Maintainers: Alan Doucette <adoucette@riotgames.com>
                     Tom Friedhof <tom@activelamp.com>


A tournament building module.

The purpose of this module is to abstract the functionality around building 
tournaments in a way that is fully customizable and extendable.

- Tournament formats are ctools plugins.
- Contestants can be any entity object.
- Tournaments, matches, games, and series are all fieldable/bundle-able entities through Entity API.
- Pathauto support.
- Uses Rules for key decisions for easy customization.
- Uses Relation for maintaining entity relationships so the content author does not have to.
- Views support.

Development sponsored by Riot Games.

OTHER SOLUTIONS
---------------
Tournament: Makes many assumptions and creates a structure that fits very little use cases.
Bracket: Only creates a bracket which should be determined by the tournament type plugin.

INSTALLATION
------------

Place the entire contents of this directory in sites/all/modules/tourney. Also
download the dependencies of this module: ctools, entity, relation, rules,
views, and views_bulk_operations. Enable the tourney module.

STEPS TO CREATING A TOURNAMENT
------------------------------

- Navigate to admin/content/tourney and click Add Tournament.
- Type the name of your tournament, how many players are playing, and the
  style of tournament.
- Click the save button.

Tourney module creates a tournament based on the information input, and renders
a bracket for single and double elimination tournaments, and a grid layout for
roundrobin tournaments.

TECHNICAL DETAILS
-----------------

The tourney module creates three new entities
  - Tournament
  - Match
  - Game

Tournament entity has a relation to the Match entity, which has a relation to
the Game entity. This is all handled using relation.module.

On creation of a tournament, every match of the tournament is created along
with the first game of the match. Tourney module uses the Rules module to
manage creating new games and moving players to the next match entity once a
match is complete.

Any entity can be a contestant in the tournament. Tourney module uses relation
module to associate contestants with matches. The default entities that can be
contestants in tournaments are users and nodes. You can configure which
entities can compete here: admin/config/tourney.

The admin interface at admin/content/tourney utilizes views bulk operations to
view tournaments, drill down to matches in a tournament, and games within a
match.

All tournament logic are in ctools plugins in the plugins/tourney_formats
directory. The BaseFormatControllers directory holds the base classes that the
plugins that ship with this module use. See the README.txt in the plugins/tourney_formats
directory for more details on how to create a new plugin.

PLUGIN ARCHITECTURE
-------------------

The plugins are built as standalone logic that extend the TourneyController in
this module. TourneyController is what ties the plugin logic found in the
BaseFormatControllers with Drupal and the entity system. The actual plugins
can be used without entities, by instantiating a plugin object and calling
render on that object.

    $tournament = SingleEliminationController(8);
    print $tournament->render();
    
This will render an eight person bracket without using any entities. Users will
likely not use the module this way, instead using the tourney/tournament/add
page to create tournaments. This example is for illustration purposes only, to
demonstrate what plugins using the TourneyController are responsible for:

1. Define the structure data of a tournament.
2. Define the match data in a tournament
3. Provide data (and logic) of what the next and previous match is for both
   winners and losers of a match.
4. How to render the tournament.

