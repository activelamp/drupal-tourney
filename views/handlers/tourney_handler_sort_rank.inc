<?php

/**
 * @file
 * Custom views sort handler that handles NULL ranks properly.
 */

class tourney_handler_sort_rank extends views_handler_sort {
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    
    $form['null_last'] = array(
      '#title' => t('Place NULL values last (Database Required)'),
      '#description' => 'Uses ISNULL() function in database to do this.',
      '#type' => 'checkbox',
      '#default_value' => $this->options['null_last'],
    );
  }
  
  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
   function options_submit(&$form, &$form_state) {
     $this->options['null_last'] = $form_state['values']['options']['null_last'];
   }
   
   /**
    * Called to add the sort to a query.  Adds ISNULL() to the alias parameter.
    */
   function query() {
     $field_alias = $this->query->add_field($this->table_alias, 'ISNULL('.$this->real_field.')', 'nullfield');
     
     $this->ensure_my_table();
     // Allows for NULL values to be placed last.
     $this->query->add_orderby($this->table_alias, NULL, 'ASC', $field_alias);
     // Add the field.
     $this->query->add_orderby($this->table_alias, $this->real_field, $this->options['order']);
   }
}
