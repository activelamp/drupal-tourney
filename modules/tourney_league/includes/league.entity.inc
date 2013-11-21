<?php

class LeagueEntity extends Entity {
  public function __construct($values = array(), $entityType = NULL) {
    parent::__construct($values, $entityType);
    // @todo: Remove this hardcoded bundle.
    $this->type = "lol";
  }
}
