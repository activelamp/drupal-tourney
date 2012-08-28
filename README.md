Tourney Module for Drupal
=========================

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

Other solutions:
Tournament: Makes many assumptions and creates a structure that fits very little use cases.
Bracket: Only creates a bracket which should be determined by the tournament type plugin.

### This is a development sandbox for http://drupal.org/project/tourney ###