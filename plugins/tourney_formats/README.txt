CONTENTS OF THIS FILE
---------------------

 * Introduction
 * Installation
 * Design Decisions


INTRODUCTION
------------

The base classes provided in the BaseFormatControllers directory provide base 
functionality for certian types of tournaments. Module developers can extend 
these classes in their own module to provide more specific functionality. The
plugin system utilizes CTools plugins to define new plugin formats.


INSTALLATION
------------

Module developers can define new formats by creating a tourney_format plugin in
their module. See the CTools plugin documentation on steps to creating new plugins.

The plugin created should provide base information about the plugin,
including what controller class to call when building. Below is the plugin defined
for Single Elimination. The 'controller' key is what does all the magic.

$plugin = array(
  'name' => t('Single Elimination Tournament'),
  'machine name' => 'single_elim',
  'description' => t('This is the description of the tournament'),
  'weight' => 0,
  'total games' => 5,
  'controller' => 'SingleEliminationController',
);


DESIGN DECISIONS
----------------
