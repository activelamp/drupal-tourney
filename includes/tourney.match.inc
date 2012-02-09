<?php

/**
 * @file
 * Defines the wrapper functions for Entity API CRUD specifically for the match
 * entity. To used these function make sure you module_load_include().
 * 
 */

/**
 * Show the tournament page.
 */
function tourney_match_page_view($tourney) {
  return $tourney->view('full', 'en', 'page');
}