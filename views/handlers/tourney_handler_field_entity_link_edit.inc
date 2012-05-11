<?php
/**
 * Field handler to present a link node edit.
 *
 * @ingroup views_field_handlers
 */
class tourney_handler_field_entity_link_edit extends views_handler_field_node_link {

  /**
   * Renders the link.
   */
  function render_link($entity, $values) {
    // Ensure user has access to edit this tournament.
    if (!user_access('edit tourney')) {
      return;
    }
    $entity_info = entity_get_info($this->entity_type);
      
    $this->options['alter']['make_link'] = TRUE;
    $this->options['alter']['path'] = $entity->uri . "/edit";
    $this->options['alter']['query'] = drupal_get_destination();

    $text = !empty($this->options['text']) ? $this->options['text'] : t('edit');
    return $text;
  }
}