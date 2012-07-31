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
    // find request handlers
    $handlers = _islandora_solr_get_handlers($solr_url);

    // check if dismax is allowed
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

  // set form array
  $form = array();

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
    '#size' => 40,
    '#weight' => -1,
    '#description' => t('The url of the Solr installation.  Defaults to localhost:8080/solr.'),
    '#default_value' => $solr_url,
    '#required' => TRUE,
    '#ajax' => array(
      'callback' => 'update_solr_url',
      'wrapper' => 'solr-url',
      'effect' => 'fade',
      'event' => 'blur',
      'progress' => array('type' => 'throbber'),
    ),
  );
  // hidden submit button
  $form['solr_ajax_wrapper']['refresh_page'] = array(
    '#type' => 'submit',
    '#value' => t('Test connection'),
    '#attributes' => array('class' => array('refresh-button')),
    '#submit' => array('solr_admin_refresh'),
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
        'callback' => 'update_solr_url',
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
        '#markup' => (isset($profile['configuration']) AND $profile['configuration'] != '') ? l(t('configure'), $profile['configuration']) : '',
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
  $form['default_display_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default display settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['default_display_settings']['islandora_solr_result_fields'] = array(
    '#type' => 'textarea',
    '#title' => t('Labels for returned fields'),
    '#description' => t('Set labels for fields returned from query.
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.title ~ Title</strong>'),
    '#default_value' => variable_get('islandora_solr_result_fields', 'dc.subject ~ Subject,dc.type ~ Type'),
    '#wysiwyg' => FALSE,
  );
  $form['default_display_settings']['islandora_solr_limit_result_fields'] = array(
    '#type' => 'checkbox',
    '#title' => t('Limit results to fields listed above?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_limit_result_fields', 0),
    '#description' => t('If checked, results displayed will be limited to the fields specified above.<br /><strong>Note:</strong> Some display profiles may not honour this value.'),
  );
  $form['default_display_settings']['islandora_solr_snippet_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Snippet field'),
    '#default_value' => variable_get('islandora_solr_snippet_field', ''),
    '#description' => t("If a match is found on this field, a snippet of text will be returned, with the search term highlighted.<br />
      An optional friendly label may inserted using the following pattern <strong>dsm.Text ~ Full Text</strong><br />
      Use an asterisk (<strong>*</strong>) for all fields, leave blank to disable highlighting.<br />
      <strong>Note:</strong><em> This feature is not supported by all display profiles.</em> "),
    '#wysiwyg' => FALSE,
  );
  $form['default_display_settings']['islandora_solr_num_of_results'] = array(
    '#type' => 'textfield',
    '#title' => t('Results per page'),
    '#size' => 5,
    '#description' => t('The number of results to show per page.'),
    '#default_value' => variable_get('islandora_solr_num_of_results', '20'),
  );
  $form['default_display_settings']['sort'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sort'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['default_display_settings']['sort']['islandora_solr_sort_terms'] = array(
    '#type' => 'textarea',
    '#title' => t('Sort terms'),
    '#description' => t("Indicate what fields should appear in the <strong>Islandora sort block</strong>.<br />
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.title ~ Title</strong><br />
      Review the schema.xml to see what terms are available. A list of the fields available when using the schema.xml packaged with the Islandora Solr
      module is available in Islandora's online documentation. <br /><strong>Important:</strong> Not all solr fields can be used for sorting. For more info, check the <a href='!sorturl' target='_blank'>solr documentation</a> on sorting.", array('!sorturl' => 'http://wiki.apache.org/solr/CommonQueryParameters#sort')),
    '#default_value' => variable_get('islandora_solr_sort_terms', 'score ~ Relevance, PID ~ PID'),
    '#wysiwyg' => FALSE,
  );



  // facet settings
  $form['facet_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Facet settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['facet_settings']['islandora_solr_facets'] = array(
    '#type' => 'textarea',
    '#title' => t('Facet fields'),
    '#description' => t("Indicate which fields will appear in the <strong>Islandora Facet Block</strong>.<br />
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.title ~ Title</strong> "),
    '#default_value' => variable_get('islandora_solr_facets', 'dc.subject ~ Subject,dc.type ~ Type'),
    '#wysiwyg' => FALSE,
  );
  $form['facet_settings']['islandora_solr_facet_min_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum limit'),
    '#size' => 5,
    '#description' => t('Minimum results required to display a facet'),
    '#default_value' => variable_get('islandora_solr_facet_min_limit', '2'),
  );
  $form['facet_settings']['islandora_solr_facet_soft_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Soft limit'),
    '#size' => 5,
    '#description' => t('The number which should be displayed initially. If there are more, then the a "Show more" button will allow the rest up to the value below to be displayed. Use 0 to disable.'),
    '#default_value' => variable_get('islandora_solr_facet_soft_limit', '10'),
  );
  $form['facet_settings']['islandora_solr_facet_max_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum limit'),
    '#size' => 5,
    '#description' => t('Set the maximum number of terms that should be returned to the user.<br />
      For example, if there are 100 possible subjects in a faceted result, you may wish to return only the top 10.'),
    '#default_value' => variable_get('islandora_solr_facet_max_limit', '20'),
  );

  $form['facet_settings']['facet_dates'] = array(
    '#type' => 'fieldset',
    '#title' => t('Date facets'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['facet_settings']['facet_dates']['islandora_solr_facet_date'] = array(
    '#type' => 'textarea',
    '#title' => t('Date field'),
    '#description' => t("Indicate which date field will appear in the <strong>Islandora Facet Block</strong>.<br />
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.date ~ Date</strong> "),
    '#default_value' => variable_get('islandora_solr_facet_date' , NULL),
  );
  $form['facet_settings']['facet_dates']['islandora_solr_facet_date_start'] = array(
    '#type' => 'textfield',
    '#title' => t('Date start'),
    '#description' => t('The lower bound for the first date range for all Date Faceting on this field. This should be a single date expression which may use the <a href="!url">DateMathParser</a> syntax.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
    '#default_value' => variable_get('islandora_solr_facet_date_start' , 'NOW/YEAR-20YEARS'),
  );
  $form['facet_settings']['facet_dates']['islandora_solr_facet_date_end'] = array(
    '#type' => 'textfield',
    '#title' => t('Date end'),
    '#description' => t('The minimum upper bound for the last date range for all Date Faceting on this field. This should be a single date expression which may use the <a href="!url">DateMathParser</a> syntax.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
    '#default_value' => variable_get('islandora_solr_facet_date_end' , 'NOW'),
  );
  $form['facet_settings']['facet_dates']['islandora_solr_facet_date_gap'] = array(
    '#type' => 'textfield',
    '#title' => t('Date gap'),
    '#description' => t('The size of each date range expressed as an interval to be added to the lower bound using the <a href="!url">DateMathParser</a> syntax.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
    '#default_value' => variable_get('islandora_solr_facet_date_gap' , '+1YEAR'),
  );
  $form['facet_settings']['facet_dates']['islandora_solr_facet_date_format'] = array(
    '#type' => 'textfield',
    '#title' => t('Date format'),
    '#description' => t('The format of the date as it will be displayed in the facet block. Use the <a href="!url">PHP date()</a> formatting.', array('!url' => 'http://lucene.apache.org/solr/api/org/apache/solr/util/DateMathParser.html')),
    '#default_value' => variable_get('islandora_solr_facet_date_format' , 'Y'),
  );




  // advanced search block
  $form['advanced_search_block'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced search block'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['advanced_search_block']['islandora_solr_searchterms'] = array(
    '#type' => 'textarea',
    '#title' => t('Search terms'),
    '#description' => t("Indicate what fields should appear in the dropdown menu of terms for the <strong>Advanced Search Block</strong>.<br />
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.title ~ Title</strong><br />
      Review the schema.xml to see what terms are available. A list of the fields available when using the schema.xml packaged with the Islandora Solr
      module is available in Islandora's online documentation."),
    '#default_value' => variable_get('islandora_solr_searchterms', 'dc.title ~ Title,dc.subject ~ Subject'),
    '#wysiwyg' => FALSE,
  );
  $form['advanced_search_block']['islandora_solr_search_boolean'] = array(
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
  $form['query_defaults'] = array(
    '#type' => 'fieldset',
    '#title' => t('Query defaults'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['query_defaults']['islandora_solr_namespace_restriction'] = array(
    '#type' => 'textfield',
    '#title' => t('Limit results to specific namespaces'),
    '#size' => 30,
    '#default_value' => variable_get('islandora_solr_namespace_restriction', ''),
    '#description' => t("Enter a space or comma-separated list of namespaces ie 'demo, default' to restrict results to PIDs within those namespaces."),
  );
  $form['query_defaults']['islandora_solr_base_query'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr default query'),
    '#size' => 30,
    '#description' => t('A default query to use to browse solr results when no explicit user query is set.
      Setting a useful default query allows users to use solr to browse, without entering a query themselves.
      May be used in conjunction with a background filter, below.<br />
      Consider <strong><em>timestamp:[* TO NOW]</em></strong> or <strong><em>*:*</em></strong><br />'),
    '#default_value' => variable_get('islandora_solr_base_query', 'timestamp:[* TO NOW]'),
  );
  $form['query_defaults']['islandora_solr_base_filter'] = array(
    '#type' => 'textarea',
    '#title' => t('Solr base filter'),
    '#description' => t('Some base filters to append to all user queries -- may be used to filter results and to facilitate null-query browsing. Enter one per line. <br />
      These filters will be applied to all queries in addition to any user-selected facet filters'),
    '#default_value' => variable_get('islandora_solr_base_filter', ''),
    '#wysiwyg' => FALSE,
  );


  // debug mode
  $form['islandora_solr_debug_mode'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug mode?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_debug_mode', 0),
    '#description' => t('Dumps solr query to the screen for testing. Warning: if you have the Drupal Apache Solr module enabled alongside this one then the debug function will not work.'),
  );

  //Notice: Undefined index: #options in expand_checkboxes() (line 1930 of /Applications/MAMP/htdocs/drupal-6.22/includes/form.inc).
  $form['islandora_solr_dismax_allowed'] = array(
    '#type' => 'hidden',
    '#value' => $dismax_allowed,
  );


  // actions
  $form['actions'] = array(
    '#type' => 'actions'
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Solr Configuration'),
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
      $row[] = array('data' => drupal_render($form['weight'][$key]), 'class' => 'solr-weight');
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

  // if this function is called
  if ($form_state['clicked_button']['#id'] != 'edit-submit' AND $form_state['clicked_button']['#id'] != 'edit-reset') {
    $form_state['rebuild'] = TRUE;
    return;
  }

  // get operation
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';

  // Exclude unnecessary elements.
  unset($form_state['values']['submit'],
        $form_state['values']['reset'],
        $form_state['values']['form_id'],
        $form_state['values']['op'],
        $form_state['values']['form_token'],
        $form_state['values']['form_build_id']);

  // if the operation is 'Reset'
  if ($op == t('Reset to defaults')) {
    // loop over all submitted values
    foreach ($form_state['values'] as $key => $value) {
      // remove all values
      variable_del($key);
    }
    // also remove the default display
    variable_del('islandora_solr_primary_display');
    // also remove the request handler variable. Form item isn't always loaded
    variable_del('islandora_solr_request_handler');

    // set reset message
    drupal_set_message(t('The configuration options have been reset to their default values.')); // @TODO: these don't seem to work anymore
  }
  else {
    // save primary display table
    // old_default variable
    $old_default = variable_get('islandora_solr_primary_display', 'default');
    // new_default variable
    $new_default = $form_state['values']['islandora_solr_primary_display_table']['default'];
    // make default enabled
    $form_state['values']['islandora_solr_primary_display_table']['enabled'][$new_default] = $new_default;
    $form_state['values']['islandora_solr_primary_display_table']['enabled'][$old_default] = $old_default;
    variable_set('islandora_solr_primary_display', $new_default);

    // loop over all submitted values
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
function update_solr_url($form, $form_state) {
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
function solr_admin_refresh($form, &$form_state) {
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

