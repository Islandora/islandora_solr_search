<?php

/**
 * @file
 * Contains all admin related functionality
 */

/**
 * Function to return admin setting form
 *
 * @param type $form
 * @param type $form_state
 * @return
 *   The form array.
 */
function islandora_solr_admin_settings($form, &$form_state) {

  // add admin form css
  drupal_add_css(drupal_get_path('module', 'islandora_solr') . '/css/islandora_solr.admin.css');
  drupal_add_library('system', 'ui.dialog');
  drupal_add_js(drupal_get_path('module', 'islandora_solr') . '/js/islandora_solr.admin.js');

  // Solr connect
  // triggering handler is dismax check or no trigger at all (page load)
  if ((isset($form_state['triggering_element']) && (($form_state['triggering_element']['#name'] == 'islandora_solr_url') || ($form_state['triggering_element']['#name'] == 'islandora_solr_request_handler'))) || !isset($form_state['triggering_element'])) {

    // check for the php solr lib class
    if (!class_exists('Apache_Solr_Service')) {
      $message = t('This module requires the <a href="!url">Apache Solr php client</a>. Please install the client directory in the root directory of this module before continuing.', array('!url' => 'http://code.google.com/p/solr-php-client'));
      drupal_set_message(check_plain($message));
      return;
    }

    // get request handler
    $handler = !empty($form_state['values']['islandora_solr_request_handler']) ? $form_state['values']['islandora_solr_request_handler'] : variable_get('islandora_solr_request_handler', '');
    // get solr url
    $solr_url = !empty($form_state['values']['islandora_solr_url']) ? $form_state['values']['islandora_solr_url'] : variable_get('islandora_solr_url', 'localhost:8080/solr');
    // check if solr is available
    $solr_avail = islandora_solr_ping($solr_url);

    $dismax_allowed = FALSE;
    // if solr is available, get the request handlers
    if ($solr_avail) {
      // find request handlers (500ms)
      $handlers = _islandora_solr_get_handlers($solr_url);
      // check if dismax is allowed (400ms)
      $dismax_allowed = _islandora_solr_check_dismax($solr_url, $handler);
    }
    // get confirmation message
    if ($solr_avail) {
      $confirmation_message = '<img src="' . url('misc/watchdog-ok.png') . '"/>'
        . t('Successfully connected to Solr server at <a href="!islandora_solr_url_checked" target="_blank">!islandora_solr_url</a> <sub>(!ms ms)</sub>', array('!islandora_solr_url_checked' => islandora_solr_check_http($solr_url), '!islandora_solr_url' => $solr_url, '!ms' => number_format($solr_avail, 2)));
    }
    else {
      $confirmation_message = '<img src="' . url('misc/watchdog-error.png') . '"/> '
        . t('Unable to connect to Solr server at !islandora_solr_url', array('!islandora_solr_url' => $solr_url));
    }
  }
  // set form array
  $form = array();
  $form['islandora_solr_tabs'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => 5,
  );
  // ajax wrapper for url checking
  $form['solr_ajax_wrapper'] = array(
    '#prefix' => '<div id="solr-url">',
    '#suffix' => '</div>',
    '#type' => 'fieldset',
  );
  // solr url
  $form['solr_ajax_wrapper']['islandora_solr_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr url'),
    '#size' => 80,
    '#weight' => -1,
    '#description' => t('The url of the Solr installation.  Defaults to localhost:8080/solr.'),
    '#default_value' => $solr_url,
    '#required' => TRUE,
    '#ajax' => array(
      'callback' => '_islandora_solr_update_solr_url',
      'wrapper' => 'solr-url',
      'effect' => 'fade',
      'event' => 'blur',
      'progress' => array('type' => 'throbber'),
    ),
  );
  $form['solr_ajax_wrapper']['islandora_solr_dismax_allowed'] = array(
    '#type' => 'hidden',
    '#default_value' => $dismax_allowed,
  );
  // hidden submit button
  $form['solr_ajax_wrapper']['refresh_page'] = array(
    '#type' => 'submit',
    '#value' => t('Test connection'),
    '#attributes' => array('class' => array('refresh-button')),
    '#submit' => array('_islandora_solr_admin_refresh'),
  );
  // confirmation message
  $form['solr_ajax_wrapper']['infobox'] = array(
    '#type' => 'item',
    '#markup' => $confirmation_message,
  );
  // request handler
  // don't show form item if no request handlers are found
  if (!empty($handlers)) {
    $form['solr_ajax_wrapper']['islandora_solr_request_handler'] = array(
      '#type' => 'select',
      '#title' => t('Request handler'),
      '#options' => $handlers,
      '#description' => t('Request handlers as defined by <a href="!url">solrconfig.xml</a>', array('!url' => 'http://wiki.apache.org/solr/SolrConfigXml')),
      '#default_value' => $handler,
      '#ajax' => array(
        'callback' => '_islandora_solr_update_solr_url',
        'wrapper' => 'solr-url',
        'effect' => 'fade',
        'event' => 'change',
        'progress' => array('type' => 'throbber'),
      ),
    );
  }


  // display profiles
  $form['display_profiles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display profiles'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );
  // primary displays
  $form['display_profiles']['islandora_solr_primary_display_table'] = array(
    '#type' => 'item',
    '#title' => t('Primary display profiles'),
    '#description' => t('Preferred normal display profile for search results.  These may be provided by third-party modules.'),
    '#tree' => TRUE, // this attribute is important to return the submitted values in a deeper nested arrays in
    '#theme' => 'islandora_solr_admin_primary_display',
  );

  // get the table settings
  $primary_display_array = variable_get('islandora_solr_primary_display_table', array());
  // get all defined primary displays
  $profiles = module_invoke_all("islandora_solr_primary_display");

  // if any primary display profiles are found
  if (!empty($profiles)) {
    $profiles_sorted = array();
    // if the table settings are set, we change the order of the table rows.
    if (!empty($primary_display_array)) {
      // set weight variable
      $weight = $primary_display_array['weight'];
      // First sort by value and then sort equal values by key: http://stackoverflow.com/a/6611077/477949
      array_multisort(array_values($weight), SORT_ASC, array_keys($weight), SORT_ASC, $weight);
      // Add all previously existing profiles with a weight...
      foreach (array_intersect_key($weight, $profiles) as $key => $value) {
        $profiles_sorted[$key] = $profiles[$key];
      }
      // account for new profiles.
      foreach (array_diff_assoc($profiles, $profiles_sorted) as $key => $value) {
        $profiles_sorted[$key] = $value;
        // add weight for new profile (heaviest +1)
        $primary_display_array['weight'][$key] = end($weight) + 1;
      }
    }
    // or else use the default
    else {
      // sort by key
      ksort($profiles); // only apply when there's no sort variable available.
      $profiles_sorted = $profiles;
    }

    // table loop
    foreach ($profiles_sorted as $machine_name => $profile) {
      // incremetally add every display profile to the options array
      $options[$machine_name] = '';

      // human name
      $form['display_profiles']['islandora_solr_primary_display_table']['name'][$machine_name] = array(
        '#type' => 'item',
        '#markup' => $profile['name'],
      );
      // machine name
      $form['display_profiles']['islandora_solr_primary_display_table']['machine_name'][$machine_name] = array(
        '#type' => 'item',
        '#markup' => $machine_name,
      );
      // weight
      $form['display_profiles']['islandora_solr_primary_display_table']['weight'][$machine_name] = array(
        '#type' => 'weight',
        '#default_value' => (isset($primary_display_array['weight'][$machine_name])) ? $primary_display_array['weight'][$machine_name] : 0,
        '#attributes' => array('class' => array('solr-weight')),
      );
      // configuration url
      $form['display_profiles']['islandora_solr_primary_display_table']['configuration'][$machine_name] = array(
        '#type' => 'item',
        '#markup' => (isset($profile['configuration']) && $profile['configuration'] != '') ? l(t('configure'), $profile['configuration']) : '',
      );
    }
    // default display
    $form['display_profiles']['islandora_solr_primary_display_table']['default'] = array(
      '#type' => 'radios',
      '#options' => $options,
      '#default_value' => variable_get('islandora_solr_primary_display', 'default'),
    );
    // enabled display
    $form['display_profiles']['islandora_solr_primary_display_table']['enabled'] = array(
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => (!empty($primary_display_array)) ? $primary_display_array['enabled'] : array(),
    );
  }

  // secondary profiles
  $profiles = module_invoke_all("islandora_solr_secondary_display");
  ksort($profiles);
  foreach ($profiles as $machine_name => $profile) {
    $islandora_solr_secondary_display_options[$machine_name] = $profile['name'];
  }
  if (!empty($islandora_solr_secondary_display_options)) {
    $form['display_profiles']['islandora_solr_secondary_display'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Secondary display profiles'),
      '#options' => $islandora_solr_secondary_display_options,
      '#default_value' => variable_get('islandora_solr_secondary_display', array()),
      '#description' => "Enabled secondary output/download types for search results.",
    );
  }


  // default display settings
  $form['islandora_solr_tabs']['default_display_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default display settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  // display fields
  $terms = array(
    '#type' => 'item',
    '#title' => t('Display fields'),
    '#description' => t('Set labels for Solr fields to be included in the search results.<br /><strong>Note:</strong> it is recommended to use non-tokenized solr fields (full literal strings).'),
    '#tree' => TRUE,
    '#prefix' => '<div id="islandora-solr-result-fields-wrapper">',
    '#suffix' => '</div>',
    '#field_type' => 'result_fields',
    '#theme' => 'islandora_solr_admin_fields',
  );

  // create terms/fields
  islandora_solr_admin_settings_fields($form_state, $terms, 'result_fields');

  // result fields
  $form['islandora_solr_tabs']['default_display_settings']['islandora_solr_result_fields'] = $terms;

  // other results settings
  $form['islandora_solr_tabs']['default_display_settings']['islandora_solr_limit_result_fields'] = array(
    '#type' => 'checkbox',
    '#title' => t('Limit results to fields listed above?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_limit_result_fields', 0),
    '#description' => t('If checked, results displayed will be limited to the fields specified above. The order of the display fields is only enforced when this is enabled.<br /><strong>Note:</strong> some display profiles may not honour this value.'),
  );
  $form['islandora_solr_tabs']['default_display_settings']['islandora_solr_num_of_results'] = array(
    '#type' => 'textfield',
    '#title' => t('Results per page'),
    '#size' => 5,
    '#description' => t('The number of results to show per page.'),
    '#default_value' => variable_get('islandora_solr_num_of_results', '20'),
  );



  // sort settings
  $form['islandora_solr_tabs']['sort'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sort settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  // sort terms
  $sort_terms = array(
    '#type' => 'item',
    '#title' => t('Sort fields'),
    '#description' => t('Indicate what fields should appear in the <strong>Islandora sort block</strong>. To sort on relevancy, use the \'score\' field.<br /><strong>Note:</strong> not all fields are sortable. For more info check the <a href="!url">Solr documentation</a>.', array('!url' => 'http://wiki.apache.org/solr/CommonQueryParameters#sort')),
    '#tree' => TRUE,
    '#prefix' => '<div id="islandora-solr-sort-fields-wrapper">',
    '#suffix' => '</div>',
    '#field_type' => 'sort_fields',
    '#theme' => 'islandora_solr_admin_fields',
  );

  // create terms/fields
  islandora_solr_admin_settings_fields($form_state, $sort_terms, 'sort_fields');

  // sort fields
  $form['islandora_solr_tabs']['sort']['islandora_solr_sort_fields'] = $sort_terms;





  // facet settings
  $form['islandora_solr_tabs']['facet_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Facet settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  // facet terms
  $facet_terms = array(
    '#type' => 'item',
    '#title' => t('Facet fields'),
    '#description' => t('Indicate what fields should appear in as <strong>facets</strong>.<br /><strong>Note:</strong> it is recommended to use non-tokenized solr fields (full literal strings).'),
    '#tree' => TRUE,
    '#prefix' => '<div id="islandora-solr-facet-fields-wrapper">',
    '#suffix' => '</div>',
    '#field_type' => 'facet_fields',
    '#theme' => 'islandora_solr_admin_fields',
  );

  // create terms/fields
  islandora_solr_admin_settings_fields($form_state, $facet_terms, 'facet_fields');

  // facet fields
  $form['islandora_solr_tabs']['facet_settings']['islandora_solr_facet_fields'] = $facet_terms;



  $form['islandora_solr_tabs']['facet_settings']['islandora_solr_facet_min_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum limit'),
    '#size' => 5,
    '#description' => t('Minimum results required to display a facet'),
    '#default_value' => variable_get('islandora_solr_facet_min_limit', '2'),
  );
  $form['islandora_solr_tabs']['facet_settings']['islandora_solr_facet_soft_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Soft limit'),
    '#size' => 5,
    '#description' => t('The number which should be displayed initially. If there are more, then the a "Show more" button will allow the rest up to the value below to be displayed. Use 0 to disable.'),
    '#default_value' => variable_get('islandora_solr_facet_soft_limit', '10'),
  );
  $form['islandora_solr_tabs']['facet_settings']['islandora_solr_facet_max_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum limit'),
    '#size' => 5,
    '#description' => t('Set the maximum number of terms that should be returned to the user. For example, if there are 100 possible subjects in a faceted result, you may wish to return only the top 10.'),
    '#default_value' => variable_get('islandora_solr_facet_max_limit', '20'),
  );


  // advanced search block
  $form['islandora_solr_tabs']['advanced_search_block'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced search block'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $search_terms = array(
    '#type' => 'item',
    '#title' => t('Search terms'),
    '#description' => t('Indicate what fields should appear in the dropdown menu of terms for the <strong>Advanced Search Block</strong>.<br /><strong>Note:</strong> It is recommended to use tokenized fields.'),
    '#tree' => TRUE,
    '#prefix' => '<div id="islandora-solr-search-fields-wrapper">',
    '#suffix' => '</div>',
    '#field_type' => 'search_fields',
    '#theme' => 'islandora_solr_admin_fields',
  );

  // create terms/fields
  islandora_solr_admin_settings_fields($form_state, $search_terms, 'search_fields');

  // search fields
  $form['islandora_solr_tabs']['advanced_search_block']['islandora_solr_search_fields'] = $search_terms;


  $form['islandora_solr_tabs']['advanced_search_block']['islandora_solr_search_boolean'] = array(
    '#type' => 'radios',
    '#title' => t('Default boolean operator'),
    '#default_value' => variable_get('islandora_solr_search_boolean', 'user'),
    '#options' => array(
      'user' => t('User configurable'),
      'AND' => t('AND'),
      'OR' => t('OR')
    ),
    '#description' => t('Select a default boolean operator for the search query. User configurable exposes a dropdown which gives the user the choice between AND, OR and NOT.'),
  );




  // query defaults
  $form['islandora_solr_tabs']['query_defaults'] = array(
    '#type' => 'fieldset',
    '#title' => t('Query defaults'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['islandora_solr_tabs']['query_defaults']['islandora_solr_namespace_restriction'] = array(
    '#type' => 'textfield',
    '#title' => t('Limit results to specific namespaces'),
    '#size' => 30,
    '#default_value' => variable_get('islandora_solr_namespace_restriction', ''),
    '#description' => t("Enter a space or comma-separated list of namespaces ie 'demo, default' to restrict results to PIDs within those namespaces."),
  );
  $form['islandora_solr_tabs']['query_defaults']['islandora_solr_base_query'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr default query'),
    '#size' => 30,
    '#description' => t('A default query to use to browse solr results when no explicit user query is set.
      Setting a useful default query allows users to use solr to browse, without entering a query themselves.
      May be used in conjunction with a background filter, below.<br />
      Consider <strong>timestamp:[* TO NOW]</strong> or <strong>*:*</strong><br />'),
    '#default_value' => variable_get('islandora_solr_base_query', 'timestamp:[* TO NOW]'),
  );
  $form['islandora_solr_tabs']['query_defaults']['islandora_solr_base_filter'] = array(
    '#type' => 'textarea',
    '#title' => t('Solr base filter'),
    '#description' => t('Some base filters to append to all user queries -- may be used to filter results and to facilitate null-query browsing. Enter one per line. <br />
      These filters will be applied to all queries in addition to any user-selected facet filters'),
    '#default_value' => variable_get('islandora_solr_base_filter', ''),
    '#wysiwyg' => FALSE,
  );

  // more
  $form['islandora_solr_tabs']['other'] = array(
    '#type' => 'fieldset',
    '#title' => t('Other'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  // debug mode
  $form['islandora_solr_tabs']['other']['islandora_solr_debug_mode'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug mode?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_debug_mode', 0),
    '#description' => t('Dumps solr query to the screen for testing. Warning: if you have the Drupal Apache Solr module enabled alongside this one then the debug function will not work.'),
    '#weight' => 6,
  );

  // The content of the popup dialog.
  $form['islandora_solr_admin_dialog'] = array(
    '#theme_wrappers' => array('container'),
    '#id' => 'islandora-solr-admin-dialog',
    '#weight' => 50,
  );
  $form['islandora_solr_admin_dialog']['title'] = array(
    '#markup' => '<h2 id="islandora-solr-admin-dialog-title"></h2>',
  );
  $form['islandora_solr_admin_dialog']['body'] = array(
    '#theme_wrappers' => array('container'),
    '#id' => 'islandora-solr-admin-dialog-body',
    '#markup' => t('Default dialog text'),
  );



  // actions
  $form['actions'] = array(
    '#type' => 'actions'
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Solr configuration'),
    '#weight' => 0,
    '#submit' => array('_islandora_solr_admin_settings_submit'),
    '#validate' => array('_islandora_solr_admin_settings_validate'),
  );
  $form['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset to defaults'),
    '#weight' => 1,
    '#submit' => array('_islandora_solr_admin_settings_submit'),
  );

  if (!empty($_POST) && form_get_errors()) {
    drupal_set_message(t('The settings have not been saved because of the errors.'), 'error');
  }
  return $form;
}

/**
 * Generates fields for the admin fields table.
 *
 * First collects values to populate the table, then render the fields.
 * @param array $form_state
 *   The form_state array of the form. Includes submitted values.
 * @param array $terms
 *   Terms array passed by reference. A base form element for the table is
 *   already created and new form elements are added in this function.
 * @param string $field_type
 *   An identifier for the field table which is being created.
 */
function islandora_solr_admin_settings_fields(&$form_state, &$terms, $field_type) {

  // set variables
  $field_type_class = str_replace('_', '-', $field_type);

  // Ways to populate the fields table
  // 1. Saved values from the DB
  // 2. Triggered element
  // 2.a. Add field
  // 2.b. Remove field
  // 2.c. Configure field // prepare array to populate dialog
  // 2.d. Dialog submit // process values from modal dialog: add variables to form_state array and update the table info

  // 1. Saved values from the DB
  if ($form_state['rebuild'] == FALSE) {

    // get values from DB
    $records = islandora_solr_get_fields($field_type, FALSE, FALSE); // unfiltered and not simplified

    // set fields to populate the admin table
    $fields = array();
    foreach ($records as $key => $record) {
      $fields[$key] = array(
        'solr_field' => $record['solr_field'],
        'field_type' => $record['field_type'],
        'weight' => $record['weight'],
      );
    }
    // move solr_field_settings into the form state
    foreach ($records as $key => $value) {
      $solr_field = $value['solr_field'];
      $solr_field_settings = $value['solr_field_settings'];
      $form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field] = $solr_field_settings;
    }
  }

  // 2. Triggered element (Add item or remove)
  if (isset($form_state['triggering_element']) && $form_state['triggering_element']['#field_type'] == $field_type) {

    // unset terms from input array
    // http://drupal.stackexchange.com/questions/14855/how-do-i-dynamically-fill-a-textfield-with-ajax/16576#16576
    $form_state['values']['islandora_solr_' . $field_type]['terms'] = $form_state['input']['islandora_solr_' . $field_type]['terms'];
    unset($form_state['input']['islandora_solr_' . $field_type]);

    // 2.a add field
    if ($form_state['triggering_element']['#name'] == $field_type . '_add_more') {
      // get new solr field value
      $add_solr_field = $form_state['values']['islandora_solr_' . $field_type]['add_solr_field'];
      // check if the value is empty
      $solr_field_error = NULL;
      if (empty($add_solr_field)) {
        $solr_field_error = t('Field cannot be empty.');
      }
      // check if value is already added
      $check_terms = $form_state['values']['islandora_solr_' . $field_type]['terms'];
      foreach ($check_terms as $key => $term) {
        if ($term['solr_field'] == $add_solr_field) {
          $solr_field_error = t('Solr term can only be added once.');
        }
      }
      // add new field to values
      if ($solr_field_error == NULL) {
        $form_state['values']['islandora_solr_' . $field_type]['terms'][] = array('solr_field' => $add_solr_field);
      }
    }
    // 2.b. remove field
    if ($form_state['triggering_element']['#value'] == t('Remove')) {
      // triggered field #
      $field = $form_state['triggering_element']['#field'];
      $solr_field = $form_state['values']['islandora_solr_' . $field_type]['terms'][$field]['solr_field'];
      // remove field
      array_splice($form_state['values']['islandora_solr_' . $field_type]['terms'], $field, 1);
      // remove field values
      unset($form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field]);
    }
    // 2.c. configure field
    if ($form_state['triggering_element']['#value'] == t('Configure')) {
      // triggered field #
      $field = $form_state['triggering_element']['#field'];
      // get field + dialog values and merge
      $values = $form_state['values']['islandora_solr_' . $field_type]['terms'][$field];
      $solr_field = $form_state['values']['islandora_solr_' . $field_type]['terms'][$field]['solr_field'];
      $solr_field_settings = $form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field];
      $values = array_merge_recursive($values, $solr_field_settings);
      // prepare dialog values
      $dialog_id = 'edit-islandora-solr-' . $field_type_class . '-terms-' . $field . '-dialog-button';
      $form_state['dialog'] = array(
        'solr_field' => $solr_field,
        'field_type' => $field_type,
        'dialog_id' => $dialog_id,
        'values' => $values,
      );
    }
    // 2.d. Dialog submit
    if ($form_state['triggering_element']['#value'] == t('Dialog')) {
      // triggered field #
      $field = $form_state['triggering_element']['#field'];
      // get solr_field
      $solr_field = $form_state['values']['islandora_solr_' . $field_type]['terms'][$field]['solr_field'];
      // get dialog values from recent dialog submit
      $dialog_values = $form_state['input']['_dialog_values'] = json_decode($form_state['input']['_dialog_values'], TRUE);
      // save the dialog values in the form state
      $form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field] = _islandora_solr_handle_solr_field_settings($dialog_values, $field_type);
    }
  }

  // prepare fields array
  if (isset($form_state['values']['islandora_solr_' . $field_type]['terms'])) {
    $fields = $form_state['values']['islandora_solr_' . $field_type]['terms'];
  }

  // if no fields available set string
  // @TODO: what's the use of this? OR !isset($form_state['triggering_element'])
  if (empty($fields)) {
    $terms['no_fields'] = array(
      '#markup' => t('No fields defined'),
    );
  }
  // else render fields
  else {
    // set variable
    $term = array();
    $count = count($fields);
    // loop over all fields
    foreach ($fields as $key => $value) {
      // solr field
      $solr_field = $value['solr_field'];

      // handler
      $term['draggable_handler'] = array(
        '#type' => 'item',
        '#markup' => '',
      );
      // solr field
      $term['solr_field_render'] = array(
        '#type' => 'item',
        '#markup' => isset($value['solr_field']) ? $value['solr_field'] : '',
      );
      $term['solr_field'] = array(
        '#type' => 'hidden',
        '#value' => isset($value['solr_field']) ? $value['solr_field'] : '',
      );
      // settings
      $solr_field_settings = NULL;
      if (isset($form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field]['label'])) {
        $solr_field_settings = $form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field]['label'];
      }
      $term['solr_field_settings'] = array(
        '#markup' => $solr_field_settings ? t('Label') . ': ' . $solr_field_settings : '',
      );
      $term['weight'] = array(
        '#type' => 'weight',
        '#default_value' => isset($value['weight']) ? $value['weight'] : $key,
        '#delta' => $count >= 10 ? $count : 10,
        '#attributes' => array('class' => array('solr-weight-' . $field_type_class)),
      );
      // configure
      $term['configure'] = array(
        '#type' => 'link',
        '#title' => t('configure'),
        '#href' => '#',
        '#options' => array('attributes' => array('class' => 'islandora-solr-configure-link')),
      );
      $term['configure_button'] = array(
        '#type' => 'submit',
        '#value' => t('Configure'),
        '#field' => $key,
        '#field_type' => $field_type,
        '#attributes' => array('class' => array('islandora-solr-configure-submit')),
        '#submit' => array('_islandora_solr_update_fields_submit'),
        '#name' => $field_type . '_configure_' . $key,
        '#id' => 'edit-islandora-solr-' . $field_type_class . '-terms-' . $key . '-configure-button',
        '#ajax' => array(
          'callback' => '_islandora_solr_admin_settings_field_configure',
          'wrapper' => 'islandora-solr-' . $field_type_class . '-wrapper',
          'effect' => 'none',
          'event' => 'click',
          'progress' => array('message' => '', 'type' => 'throbber'),
        ),
      );
      // remove
      $term['remove'] = array(
        '#type' => 'link',
        '#title' => t('remove'),
        '#href' => '#',
        '#options' => array('attributes' => array('class' => 'islandora-solr-remove-link')),
      );
      $term['remove_button'] = array(
        '#type' => 'submit',
        '#value' => t('Remove'),
        '#field' => $key,
        '#field_type' => $field_type,
        '#attributes' => array('class' => array('islandora-solr-remove-submit')),
        '#submit' => array('_islandora_solr_update_fields_submit'),
        '#name' => $field_type . '_remove_' . $key,
        '#id' => 'edit-islandora-solr-' . $field_type_class . '-terms-' . $key . '-remove-button',
        '#ajax' => array(
          'callback' => '_islandora_solr_update_fields',
          'wrapper' => 'islandora-solr-' . $field_type_class . '-wrapper',
          'effect' => 'none',
          'event' => 'click',
          'progress' => array('message' => '', 'type' => 'throbber'),
        ),
      );
      // dialog submit
      $term['dialog_button'] = array(
        '#type' => 'submit',
        '#value' => t('Dialog'),
        '#field' => $key,
        '#field_type' => $field_type,
        '#attributes' => array('class' => array('islandora-solr-dialog-submit')),
        '#submit' => array('_islandora_solr_update_fields_submit'),
        '#name' => $field_type . '_dialog_' . $key,
        '#id' => 'edit-islandora-solr-' . $field_type_class . '-terms-' . $key . '-dialog-button',
        '#ajax' => array(
          'callback' => '_islandora_solr_update_fields',
          'wrapper' => 'islandora-solr-' . $field_type_class . '-wrapper',
          'effect' => 'none',
          'event' => 'click',
          'progress' => array('message' => '', 'type' => 'throbber'),
        ),
      );
      $terms['terms'][] = $term;
    }
  }
  // add solr field
  $terms['add_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Add another item'),
    '#size' => 45,
    '#autocomplete_path' => 'islandora_solr/autocomplete_luke',
    '#default_value' => '',
  );
  if (isset($solr_field_error) && !empty($solr_field_error)) {
    $terms['add_solr_field']['#title'] .= '<br /><span class="solr-admin-error">' . $solr_field_error . '</span>';
    $terms['add_solr_field']['#attributes'] = array('class' => array('error'));
  }

  // add more button
  $terms['add_more'] = array(
    '#type' => 'button',
    '#value' => t('Add'),
    '#attributes' => array('class' => array('islandora-solr-add-more-submit')),
    '#submit' => array('_islandora_solr_update_fields_submit'),
    '#name' => $field_type . '_add_more',
    '#field_type' => $field_type,
    '#id' => 'edit-islandora-solr-' . $field_type_class . '-add-more',
    '#ajax' => array(
      'callback' => '_islandora_solr_update_fields',
      'wrapper' => 'islandora-solr-' . $field_type_class . '-wrapper',
      'event' => 'click',
      'effect' => 'none',
    ),
  );
}

/**
 * Theme function to create an admin table for result fields
 * @param array $variables
 *   Array containing form elements to be themed.
 * @return string
 *   Rendered table
 */
function theme_islandora_solr_admin_fields($variables) {
  // set form variable
  $form = $variables['form'];
  // get field type
  $field_type = $form['#field_type'];
  // render class name from field type
  $field_type_class = str_replace('_', '-', $field_type);

  // set rows array
  $rows = array();
  // if no_fields is set, we don't render all the fields
  if (isset($form['no_fields'])) {
    $row = array();
    $row[] = array('data' => drupal_render($form['no_fields']), 'colspan' => 10);
    // add to rows
    $rows[] = array('data' => $row);
  }
  else {
    // loop over values
    foreach ($form['terms'] as $key => $element) {
      if (is_array($element) && element_child($key)) {
        $row = array();
        $row[] = array('data' => drupal_render($form['terms'][$key]['draggable_handler']), 'class' => 'islandora-solr-multiple-drag');
        $row[] = array('data' => drupal_render($form['terms'][$key]['solr_field_render']), 'class' => 'islandora-solr-solr-field-render');
        $row[] = array('data' => drupal_render($form['terms'][$key]['solr_field_settings']), 'class' => 'islandora-solr-solr-field-settings');
        $row[] = array('data' => drupal_render($form['terms'][$key]['weight']));
        $row[] = array('data' => drupal_render($form['terms'][$key]['configure']), 'class' => 'islandora-solr-operations');
        $row[] = array('data' => drupal_render($form['terms'][$key]['configure_button']), 'class' => 'islandora-solr-operations islandora-solr-operations-hidden');
        $row[] = array('data' => drupal_render($form['terms'][$key]['remove']), 'class' => 'islandora-solr-operations');
        $row[] = array('data' => drupal_render($form['terms'][$key]['remove_button']), 'class' => 'islandora-solr-operations islandora-solr-operations-hidden');
        $row[] = array('data' => drupal_render($form['terms'][$key]['dialog_button']), 'class' => 'islandora-solr-operations islandora-solr-operations-hidden');
        // add to rows
        $rows[] = array('data' => $row, 'class' => array('draggable'));
      }
    }
  }
  // create header array
  $header = array();
  $header[] = array('data' => t('Solr field'), 'colspan' => 2, 'class' => 'islandora-solr-solr-field-header');
  $header[] = array('data' => t('Settings'));
  $header[] = array('data' => t('Weight'), 'class' => 'islandora-solr-solr-field-weight');
  $header[] = array('data' => t('Operations'), 'colspan' => 4);
  // Controls table
  $rows_controls = array();
  // add another item form elements
  $rows_controls[] = array(
    'data' => array(
      array('data' => drupal_render($form['add_solr_field']) . drupal_render($form['add_more']), 'colspan' => 10),
    ),
    'no_striping' => 1,
    'class' => array('add-item-controls')
  );
  // render form
  $output = '';
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'islandora-solr-' . $field_type_class, 'class' => array('islandora-solr-fields-table'))));
  $output .= theme('table', array('header' => array(), 'rows' => $rows_controls, 'attributes' => array('class' => array('islandora-solr-table-controls'))));
  $output .= drupal_render_children($form);
  // add tabledrag features
  drupal_add_tabledrag('islandora-solr-' . $field_type_class, 'order', 'sibling', 'solr-weight-' . $field_type_class);

  return $output;
}

/**
 * Theme function for the admin primary display table
 *
 * @param type $variables
 *   render element: $form
 *   Contains the form array
 * @return
 *   rendered form element
 *
 *  @see islandora_solr_admin_settings()
 */
function theme_islandora_solr_admin_primary_display($variables) {
  // set form
  $form = $variables['form'];

  $default = variable_get('islandora_solr_primary_display', 'default');
  $rows = array();

  foreach ($form['name'] as $key => $element) {
    // Do not take form control structures.
    if (is_array($element) && element_child($key)) {
      // disable checkbox for the default display, because it cannot be disabled.
      if ($key == $default) {
        $form['enabled'][$key]['#attributes']['disabled'] = 'disabled';
        $form['enabled'][$key]['#checked'] = TRUE;
      }
      $row = array();
      $row[] = array('data' => drupal_render($form['default'][$key]));
      $row[] = array('data' => drupal_render($form['enabled'][$key]));
      $row[] = array('data' => drupal_render($form['name'][$key]), 'title' => $key);
      $row[] = array('data' => drupal_render($form['machine_name'][$key]));
      $row[] = array('data' => drupal_render($form['weight'][$key]));
      $row[] = array('data' => drupal_render($form['configuration'][$key]));
      // add to rows
      $rows[] = array('data' => $row, 'class' => array('draggable'));
    }
  }
  // Individual table headers.
  // default | enabled | name | Machine readable name |  weight | configuration
  $header = array();
  $header[] = array('data' => t('Default'));
  $header[] = array('data' => t('Enabled'));
  $header[] = array('data' => t('Name'));
  $header[] = array('data' => t('Machine readable name'));
  $header[] = array('data' => t('Weight'));
  $header[] = array('data' => t('Configuration'));
  // render form
  $output = '';
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'solr-primary-display-table')));
  $output .= drupal_render_children($form);
  // add tabledrag features
  drupal_add_tabledrag('solr-primary-display-table', 'order', 'sibling', 'solr-weight');

  return $output;
}

/**
 * Form validator
 *
 * @param array $form
 * @param array $form_state
 */
function _islandora_solr_admin_settings_validate($form, &$form_state) {

  $form_state['values']['islandora_solr_namespace_restriction'] = preg_replace('/:$/', '', $form_state['values']['islandora_solr_namespace_restriction']);
}

/**
 * Solr settings from submit
 *
 * @param array $form
 * @param array $form_state
 * @return NULL
 */
function _islandora_solr_admin_settings_submit($form, &$form_state) {

  // if this function is called not using submit or reset buttons
  if ($form_state['clicked_button']['#id'] != 'edit-submit' && $form_state['clicked_button']['#id'] != 'edit-reset') {
    $form_state['rebuild'] = TRUE;
    return;
  }

  // get operation
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';

  // Exclude unnecessary elements. // @TODO: remove once we assigned the values manually.
  unset($form_state['values']['submit'],
    $form_state['values']['reset'],
    $form_state['values']['form_id'],
    $form_state['values']['op'],
    $form_state['values']['form_token'],
    $form_state['values']['form_build_id'],
    $form_state['values']['refresh_page']);

  // if the operation is 'Reset'
  switch ($op) {
    case t('Save Solr configuration'):
      // save primary display table
      // old_default variable
      $old_default = variable_get('islandora_solr_primary_display', 'default');
      // new_default variable
      $new_default = $form_state['values']['islandora_solr_primary_display_table']['default'];
      // make default enabled
      $form_state['values']['islandora_solr_primary_display_table']['enabled'][$new_default] = $new_default;
      $form_state['values']['islandora_solr_primary_display_table']['enabled'][$old_default] = $old_default;
      variable_set('islandora_solr_primary_display', $new_default);

      // prepare all field variables to be saved to the DB.
      $insert_values = array();
      $field_types = array('result_fields', 'sort_fields', 'facet_fields', 'search_fields');
      foreach ($field_types as $field_type) {
        if (isset($form_state['values']['islandora_solr_' . $field_type]['terms'])) {
          $result_fields = $form_state['values']['islandora_solr_' . $field_type]['terms'];
          foreach ($result_fields as $key => $value) {
            $solr_field = $value['solr_field'];
            $solr_field_settings = array();
            if (isset($form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field])) {
              $solr_field_settings = $form_state['solr_field_settings']['islandora_solr_' . $field_type][$solr_field];
            }
            $insert_values[] = array(
              'solr_field' => $solr_field,
              'field_type' => $field_type,
              'weight' => $value['weight'],
              'solr_field_settings' => serialize($solr_field_settings),
            );
          }
        }
      }

      // clear the db fields table // @TODO: merge delete and save in one query.
      db_delete('islandora_solr_fields')->execute();
      // then populate them
      $insert = db_insert('islandora_solr_fields')->fields(array('solr_field', 'field_type', 'weight', 'solr_field_settings'));
      foreach ($insert_values as $record) {
        $insert->values($record);
      }
      $insert->execute();

      // loop over all submitted values
      unset($form_state['values']['islandora_solr_result_fields']);
      unset($form_state['values']['islandora_solr_sort_fields']);
      unset($form_state['values']['islandora_solr_facet_fields']);
      unset($form_state['values']['islandora_solr_search_fields']);
      foreach ($form_state['values'] as $key => $value) {
        // prepare variables in array
        if (is_array($value) && isset($form_state['values']['array_filter'])) {
          $value = array_keys(array_filter($value));
        }
        // set variables
        variable_set($key, $value);
      }

      // set save message
      drupal_set_message(t('The solr configuration options have been saved.'));
    break;
    case t('Reset to defaults'):
      // loop over all submitted values
      foreach ($form_state['values'] as $key => $value) {
        // remove all values
        variable_del($key);
      }
      // also remove the default display
      variable_del('islandora_solr_primary_display');
      // also remove the request handler variable. Form item isn't always loaded
      variable_del('islandora_solr_request_handler');

      // clear the db fields table
      db_delete('islandora_solr_fields')->execute();

      // set reset message
      drupal_set_message(t('The configuration options have been reset to their default values.')); // @TODO: these don't seem to work anymore
    break;
  }

  // clear caches
  cache_clear_all();
  drupal_theme_rebuild();
}

/**
 * Get available handlers
 *
 * @param String $solr_url
 *   url which points to Solr
 * @return
 *   a php array with request handlers found in solrconfig.xml of the Solr
 *   instance
 */
function _islandora_solr_get_handlers($solr_url) {
  $url = $solr_url . '/admin/file/?file=solrconfig.xml';
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  curl_setopt($ch, CURLOPT_URL, $url);
  $xml = simplexml_load_string(curl_exec($ch));
  foreach ($xml->requestHandler as $handler) {
    if ($handler['class'] == 'solr.SearchHandler') {
      $handler_name = (string) $handler['name'];
      $slashed = preg_match('/^\//', $handler_name);
      if ($handler_name != 'dismax' && $handler_name != 'partitioned' & !$slashed) {
        $handlers[$handler_name] = $handler_name;
      }
    }
  }
  return $handlers;
}

/**
 * AJAX callback from the admin form.
 * Checks url validity and refreshes request handler dropdown list
 *
 * @param type $form
 * @param array $form_state
 * @return type
 *
 * @see islandora_solr_admin_settings()
 */
function _islandora_solr_update_solr_url($form, $form_state) {
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  $form_state['rebuild'] = TRUE;

  return $form['solr_ajax_wrapper'];
}

/**
 * Submit button callback to update the solr url only
 *
 * @param type $form
 * @param array $form_state
 *
 * @see islandora_solr_admin_settings()
 */
function _islandora_solr_admin_refresh($form, &$form_state) {
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  $form_state['rebuild'] = TRUE;
}

/**
 * Checks for dismax
 *
 * @param type $solr_url
 *   url which point to solr
 * @param type $selected_handler
 *   handler to check if dismax is allowed on it
 * @return boolean
 *  TRUE if dismax is allowed, FALSE if not.
 *
 * @see islandora_solr_admin_settings()
 */
function _islandora_solr_check_dismax($solr_url, $selected_handler) {
  $dismax = FALSE;
  $url = $solr_url . '/admin/file/?file=solrconfig.xml';
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  curl_setopt($ch, CURLOPT_URL, $url);
  $xml = simplexml_load_string(curl_exec($ch));
  foreach ($xml->requestHandler as $handler) {
    if ($handler['class'] == 'solr.SearchHandler') {
      if ((string) $handler['name'] == $selected_handler) {
        foreach ($handler->lst as $first) {
          foreach ($first->str as $str) {
            if ($str['name'] == 'qf') {
              $dismax = TRUE;
            }
          }
        }
      }
    }
  }
  return $dismax;
}

/**
 * Admin autocomplete callback which returns solr fields from Luke.
 *
 * @param string $string
 *   String filled out in the autocomplete textfield.
 * @return json
 *   A json array containing the Solr luke values that contain the given string.
 */
function _islandora_solr_autocomplete_luke($string = '') {

  $luke = islandora_solr_get_luke();
  $result = array();
  // loop over luke fields
  foreach ($luke['fields'] as $term => $value) {
    // if string is found in the solr term
    if (stripos($term, $string) !== FALSE) {
      // add strong elements to highlight the found string. Search case
      // insensitive, but keep the case on replace.
      // http://stackoverflow.com/a/3489768/477949
      $term_str = preg_replace("/$string/i", "<strong>\$0</strong>", $term);
      $result[$term] = $term_str . '<strong style="position: absolute; right: 5px;">(' . $value['type'] . ')</strong>';
    }
  }
  // sort alphabetically
  ksort($result);

  drupal_json_output($result);
  exit();
}

/**
 * Submit callback function for fields. Used by 'Add field', 'Configure', 'Remove' and 'Dialog'.
 */
function _islandora_solr_update_fields_submit($form, &$form_state) {

  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax callback function for fields. Used by 'Add field', 'Remove' and 'Dialog' links/buttons.
 * Returns part of the form array that needs asynchronous updating.
 */
function _islandora_solr_update_fields($form, &$form_state) {

  // check what section the call is from
  switch ($form_state['triggering_element']['#field_type']) {
    case 'result_fields':
      return $form['islandora_solr_tabs']['default_display_settings']['islandora_solr_result_fields'];
      break;
    case 'sort_fields':
      return $form['islandora_solr_tabs']['sort']['islandora_solr_sort_fields'];
      break;
    case 'facet_fields':
      return $form['islandora_solr_tabs']['facet_settings']['islandora_solr_facet_fields'];
      break;
    case 'search_fields':
      return $form['islandora_solr_tabs']['advanced_search_block']['islandora_solr_search_fields'];
      break;
    default:
      break;
  }
}

/**
 * Ajax callback function for 'Configure' link/button.
 *
 * Passes values from the field, loads the right form and populates and triggers the modal dialog.
 */
function _islandora_solr_admin_settings_field_configure($form, &$form_state) {
  // set variables
  $variables = $form_state['dialog'];
  $field_type = $form_state['dialog']['field_type'];
  $solr_field = $form_state['dialog']['solr_field'];
  $form_function = _islandora_solr_admin_get_form_function($field_type);

  $commands = array();
  // open dialog
  $commands[] = ajax_command_invoke('#islandora-solr-admin-dialog', 'dialog', array('open'));
  // dialog title
  $title = t('Configure field:') . ' ' . $solr_field;
  $commands[] = ajax_command_invoke('#islandora-solr-admin-dialog-title', 'html', array($title));
  // dialog body
  $body = drupal_render(drupal_get_form($form_function, $variables));
  $commands[] = ajax_command_invoke('#islandora-solr-admin-dialog-body', 'html', array($body));
  // attach behaviors (re-attaches javascript)
  $commands[] = ajax_command_invoke(NULL, 'islandoraSolrAttachBehaviors', array());

  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * Ajax callback function for the modal dialog submit button.
 *
 * Closes the modal dialog and passes the values to the main form.
 */
function _islandora_solr_admin_settings_dialog($form, &$form_state) {

  $dialog_id = $form_state['dialog']['dialog_id'];

  $commands = array();
  // close dialog
  $commands[] = ajax_command_invoke('#islandora-solr-admin-dialog', 'dialog', array('close'));

  // Call function which attaches the dialog form values to Drupal.ajax.{dialog-button-id}.options.data.
  // After triggering the dialog ajax callback, the dialog values will be added to the $form_state of
  // the main form
  $data = array(
    'id' => $dialog_id,
    'values' => $form_state['input'],
  );
  $commands[] = ajax_command_invoke(NULL, 'islandoraSolrDialogValues', array($data));
  // trigger dialog button
  $commands[] = ajax_command_invoke('#' . $dialog_id, 'trigger', array('click'));

  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * Function that selects the fields from the raw input values the modal form processes through the dialog button
 * Ajax call.
 * @TODO: this should probably be removed and values we don't need maybe should be filtered out before sending it back
 *   to the main form.
 */
function _islandora_solr_handle_solr_field_settings($solr_field_settings = NULL, $field_type = NULL) {
  if (!empty($solr_field_settings) && !empty($field_type)) {
    switch ($field_type) {
      case 'result_fields':
        return array(
          'label' => $solr_field_settings['label'],
          'snippet' => $solr_field_settings['snippet'],
          'permissions' => $solr_field_settings['permissions'],
        );
        break;
      case 'sort_fields':
        return array(
          'label' => $solr_field_settings['label'],
          'permissions' => $solr_field_settings['permissions'],
        );
        break;
      case 'facet_fields':
        return array(
          'label' => $solr_field_settings['label'],
          'range_facet_select' => $solr_field_settings['range_facet_select'],
          'range_facet_start' => $solr_field_settings['range_facet_start'],
          'range_facet_end' => $solr_field_settings['range_facet_end'],
          'range_facet_gap' => $solr_field_settings['range_facet_gap'],
          'date_facet_format' => $solr_field_settings['date_facet_format'],
          'permissions' => $solr_field_settings['permissions'],
        );
        break;
      case 'search_fields':
        return array(
          'label' => $solr_field_settings['label'],
          'permissions' => $solr_field_settings['permissions'],
        );
        break;
      default:
        return FALSE;
        break;
    }
    return $form_callback;
  }
  return FALSE;
}

/**
 * Function which returns the form function name associated with a field type.
 *
 * @param string $field_type
 *   Field type to get the form function name for.
 * @return
 *   A form function name associated with the field type, or FALSE if field type is not
 *   given or does not exist.
 */
function _islandora_solr_admin_get_form_function($field_type = NULL) {
  if (!empty($field_type)) {
    switch ($field_type) {
      case 'result_fields':
        $form_callback = 'islandora_solr_admin_settings_result_fields'; // result field
        break;
      case 'sort_fields':
        $form_callback = 'islandora_solr_admin_settings_sort_fields'; // sort field
        break;
      case 'facet_fields':
        $form_callback = 'islandora_solr_admin_settings_facet_fields'; // facet field
        break;
      case 'search_fields':
        $form_callback = 'islandora_solr_admin_settings_search_fields'; // search field
        break;
      default:
        return FALSE;
        break;
    }
    return $form_callback;
  }
  return FALSE;
}

/**
 * @defgroup dialog-forms
 * @{
 * Form functions for the modal dialogs.
 */

/**
 * Form for result field settings.
 *
 * @params array $variables
 *   This parameter contains default values to be passed from the main form.
 */
function islandora_solr_admin_settings_result_fields($form, &$form_state, $variables) {
  // include this file in build_info
  form_load_include($form_state, 'inc', 'islandora_solr', 'islandora_solr.admin');
  // pass variables to form state
  $form_state['dialog'] = $variables;
  // get solr field values
  $values = $variables['values'];
  $permissions = $values['permissions'];
  $permissions_disable = _islandora_solr_permissions_disable();
  $permissions_default = _islandora_solr_permissions_default();

  $form = array();
  $form['options'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('scroll')),
    '#id' => 'islandora-solr-admin-dialog-form',
  );
  $form['options']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => $values['label'],
    '#description' => t('A human readable name.'),
  );
  $form['options']['snippet'] = array(
    '#type' => 'checkbox',
    '#title' => t('Snippet'),
    '#default_value' => $values['snippet'],
    '#description' => t('If a match is found on this field, a snippet of text will be returned, with the search term highlighted.<br /><strong>Note:</strong> this feature is not supported by all display profiles.'),
  );
  $form['options']['permissions_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Permissions'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['options']['permissions_fieldset']['permissions'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Permissions'),
    '#options' => user_roles(),
    '#default_value' => !empty($permissions) ? $permissions : $permissions_default,
    '#description' => t('Select what role can access this field.<br /><strong>Note:</strong> checkboxes may be grayed out for roles who do not have permission to search the Solr index.'),
  );
  foreach ($permissions_disable as $rid) {
    $form['options']['permissions_fieldset']['permissions'][$rid] = array(
      '#disabled' => TRUE,
    );
  }
  $form['actions'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('form-buttons')),
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Apply'),
    '#weight' => 5,
    '#field' => 'dialog_submit',
    '#field_type' => 'result_fields', // result_fields
    '#name' => 'result-fields-dialog-submit',
    '#submit' => array('_islandora_solr_update_fields_submit'),
    '#ajax' => array(
      'callback' => '_islandora_solr_admin_settings_dialog',
      'wrapper' => 'islandora-solr-admin-dialog-form',
      'effect' => 'none',
      'event' => 'click',
      'progress' => array('message' => '', 'type' => 'throbber'),
    ),
  );
  return $form;
}

/**
 * Form for sort field settings.
 *
 * @params array $variables
 *   This parameter contains default values to be passed from the main form.
 */
function islandora_solr_admin_settings_sort_fields($form, &$form_state, $variables) {
  // include this file in build_info
  form_load_include($form_state, 'inc', 'islandora_solr', 'islandora_solr.admin');

  $form_state['dialog'] = $variables;
  // get solr field values
  $values = $variables['values'];
  $permissions = $values['permissions'];
  $permissions_disable = _islandora_solr_permissions_disable();
  $permissions_default = _islandora_solr_permissions_default();

  $form = array();
  $form['options'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('scroll')),
    '#id' => 'islandora-solr-admin-dialog-form',
  );
  $form['options']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => $values['label'],
    '#description' => t('A human readable name.'),
  );
  $form['options']['permissions_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Permissions'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['options']['permissions_fieldset']['permissions'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Permissions'),
    '#options' => user_roles(),
    '#default_value' => !empty($permissions) ? $permissions : $permissions_default,
    '#description' => t('Select what role can access this field.<br /><strong>Note:</strong> checkboxes may be grayed out for roles who do not have permission to search the Solr index.'),
  );
  foreach ($permissions_disable as $rid) {
    $form['options']['permissions_fieldset']['permissions'][$rid] = array(
      '#disabled' => TRUE,
    );
  }
  $form['actions'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('form-buttons')),
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Apply'),
    '#weight' => 5,
    '#field' => 'dialog_submit',
    '#field_type' => 'result_fields', // result_fields
    '#name' => 'result-fields-dialog-submit',
    '#submit' => array('_islandora_solr_update_fields_submit'),
    '#ajax' => array(
      'callback' => '_islandora_solr_admin_settings_dialog',
      'wrapper' => 'islandora-solr-admin-dialog-form',
      'effect' => 'none',
      'event' => 'click',
      'progress' => array('message' => '', 'type' => 'throbber'),
    ),
  );
  return $form;
}

/**
 * Form for facet field settings.
 *
 * @params array $variables
 *   This parameter contains default values to be passed from the main form.
 */
function islandora_solr_admin_settings_facet_fields($form, &$form_state, $variables) {
  // include this file in build_info
  form_load_include($form_state, 'inc', 'islandora_solr', 'islandora_solr.admin');

  $form_state['dialog'] = $variables;
  // get solr field values
  $values = $variables['values'];
  $permissions = $values['permissions'];
  $permissions_disable = _islandora_solr_permissions_disable();
  $permissions_default = _islandora_solr_permissions_default();

  $form = array();
  $form['options'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('scroll')),
    '#id' => 'islandora-solr-admin-dialog-form',
  );
  $form['options']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => $values['label'],
    '#description' => t('A human readable name.'),
  );

  $form['options']['range_facet'] = array(
    '#type' => 'fieldset',
    '#id' => 'range-facet-wrapper',
    '#collapsible' => FALSE,
    '#collapsed' => TRUE,
  );

  $form['options']['range_facet']['range_facet_select'] = array( // @TODO: grey out if LUKE says it's not possible to use as a range field? Add AJAX callback to show more options?
    '#type' => 'checkbox',
    '#title' => t('Range facet'),
    '#default_value' => $values['range_facet_select'],
    '#description' => t('Whether this facet field should be configured as a Solr range facet.'),
  );

  // @TODO: check for non-ajax values
    $form['options']['range_facet']['wrapper'] = array(
      '#type' => 'container',
      '#states' => array(
        'visible' => array(
          ':input[name="range_facet_select"]' => array('checked' => TRUE)
        ),
      ),
    );
    $form['options']['range_facet']['wrapper']['range_facet_start'] = array(
      '#type' => 'textfield',
      '#title' => t('Start'),
      '#description' => t('The lower bound for the first date range for all Date Faceting on this field. This should be a single date expression which may use the <a href="!url">DateMathParser</a> syntax.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
      '#default_value' => $values['range_facet_start'] ? $values['range_facet_start'] : 'NOW/YEAR-20YEARS',
    );
    $form['options']['range_facet']['wrapper']['range_facet_end'] = array(
      '#type' => 'textfield',
      '#title' => t('End'),
      '#description' => t('The minimum upper bound for the last date range for all Date Faceting on this field. This should be a single date expression which may use the <a href="!url">DateMathParser</a> syntax.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
      '#default_value' => $values['range_facet_end'] ? $values['range_facet_end'] : 'NOW',
    );
    $form['options']['range_facet']['wrapper']['range_facet_gap'] = array(
      '#type' => 'textfield',
      '#title' => t('Gap'),
      '#description' => t('The size of each date range expressed as an interval to be added to the lower bound using the <a href="!url">DateMathParser</a> syntax.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
      '#default_value' => $values['range_facet_gap'] ? $values['range_facet_gap'] : '+1YEAR',
    );
    $form['options']['range_facet']['wrapper']['date_facet_format'] = array(
      '#type' => 'textfield',
      '#title' => t('Date format'),
      '#description' => t('The format of the date as it will be displayed in the facet block. Use the <a href="!url">PHP date()</a> formatting.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
      '#default_value' => $values['date_facet_format'] ? $values['date_facet_format'] : 'Y',
    );

  $form['options']['permissions_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Permissions'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['options']['permissions_fieldset']['permissions'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Permissions'),
    '#options' => user_roles(),
    '#default_value' => !empty($permissions) ? $permissions : $permissions_default,
    '#description' => t('Select what role can access this field.<br /><strong>Note:</strong> checkboxes may be grayed out for roles who do not have permission to search the Solr index.'),
  );
  foreach ($permissions_disable as $rid) {
    $form['options']['permissions_fieldset']['permissions'][$rid] = array(
      '#disabled' => TRUE,
    );
  }
  $form['actions'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('form-buttons')),
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Apply'),
    '#weight' => 5,
    '#field' => 'dialog_submit',
    '#field_type' => 'result_fields', // result_fields
    '#name' => 'result-fields-dialog-submit',
    '#submit' => array('_islandora_solr_update_fields_submit'),
    '#ajax' => array(
      'callback' => '_islandora_solr_admin_settings_dialog',
      'wrapper' => 'islandora-solr-admin-dialog-form',
      'effect' => 'none',
      'event' => 'click',
      'progress' => array('message' => '', 'type' => 'throbber'),
    ),
  );
  return $form;
}

/**
 * Ajax callback function for range facet checkbox.
 */
/*
function _islandora_solr_admin_settings_facet_fields_callback($form, &$form_state) {

  // resize modal
  ajax_command_invoke(NULL, 'islandoraSolrResizeModal', array());

  return $form['options']['range_facet'];
}
 */

/**
 * Form for search field settings.
 *
 * @params array $variables
 *   This parameter contains default values to be passed from the main form.
 */
function islandora_solr_admin_settings_search_fields($form, &$form_state, $variables) {
  // include this file in build_info
  form_load_include($form_state, 'inc', 'islandora_solr', 'islandora_solr.admin');

  $form_state['dialog'] = $variables;
  // get solr field values
  $values = $variables['values'];
  $permissions = $values['permissions'];
  $permissions_disable = _islandora_solr_permissions_disable();
  $permissions_default = _islandora_solr_permissions_default();

  $form = array();
  $form['options'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('scroll')),
    '#id' => 'islandora-solr-admin-dialog-form',
  );
  $form['options']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => $values['label'],
    '#description' => t('A human readable name.'),
  );
  $form['options']['permissions_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Permissions'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['options']['permissions_fieldset']['permissions'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Permissions'),
    '#options' => user_roles(),
    '#default_value' => !empty($permissions) ? $permissions : $permissions_default,
    '#description' => t('Select what role can access this field.<br /><strong>Note:</strong> checkboxes may be grayed out for roles who do not have permission to search the Solr index.'),
  );
  foreach ($permissions_disable as $rid) {
    $form['options']['permissions_fieldset']['permissions'][$rid] = array(
      '#disabled' => TRUE,
    );
  }
  $form['actions'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('form-buttons')),
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Apply'),
    '#weight' => 5,
    '#field' => 'dialog_submit',
    '#field_type' => 'result_fields', // result_fields
    '#name' => 'result-fields-dialog-submit',
    '#submit' => array('_islandora_solr_update_fields_submit'),
    '#ajax' => array(
      'callback' => '_islandora_solr_admin_settings_dialog',
      'wrapper' => 'islandora-solr-admin-dialog-form',
      'effect' => 'none',
      'event' => 'click',
      'progress' => array('message' => '', 'type' => 'throbber'),
    ),
  );
  return $form;
}

/**
 * Returns an array of role id's to disable checkboxes
 */
function _islandora_solr_permissions_disable() {
  $user_roles = array_keys(user_roles());
  $permissions_solr = array_keys(user_roles(FALSE, 'search islandora solr'));
  $permissions_disable = array_diff($user_roles, $permissions_solr);
  // if authenticated users have permission exclude all authenticated users fields from the disable list.
  if (in_array('2', $permissions_solr)) {
    foreach ($permissions_disable as $key => $rid) {
      if ($rid != '1') {
        unset($permissions_disable[$key]);
      }
    }
  }
  return $permissions_disable;
}

/**
 * Returns an array of role id's to set default values for checkboxes
 */
function _islandora_solr_permissions_default() {
  $user_roles = array_keys(user_roles());
  $permissions_solr = array_keys(user_roles(FALSE, 'search islandora solr'));
  $permissions_default = array_intersect($user_roles, $permissions_solr);
  // if authenticated users have permission include all authenticated users fields to the default list.
  if (in_array('2', $permissions_solr)) {
    foreach ($user_roles as $rid) {
      if ($rid != '1' && !in_array($rid, $permissions_default)) {
        $permissions_default[] = $rid;
      }
    }
  }
  return $permissions_default;
}

/**
 * @} End of "defgroup dialog-forms".
 */
