<?php

/**
 * Function to return admin setting form
 * @return array 
 */
function islandora_solr_admin_settings(&$form_state) {

  //checks for existence of PHP Solr client.
  module_load_include('php', 'islandora_solr_search', 'SolrPhpClient/Apache/Solr/Service');
  if (!class_exists('Apache_Solr_Service')) {
    drupal_set_message("This module requires the " . l('Apache Solr php client', 'http://code.google.com/p/solr-php-client') . '.
      Please install the client directory in the root directory of this module before continuing.');
    return;
  }
  $handlers['standard'] = 'standard';

 $handler = !empty($form_state['values']['islandora_solr_search_block_request_handler']) ? $form_state['values']['islandora_solr_search_block_request_handler'] : variable_get('islandora_solr_search_block_request_handler', 'standard');
  $solr_url = !empty($form_state['values']['islandora_solr_search_block_url']) ? $form_state['values']['islandora_solr_search_block_url'] : variable_get('islandora_solr_search_block_url', 'localhost:8080/solr');
  $solr_avail = solr_available($solr_url);
  if ($solr_avail) {
    $handlers = get_handlers($solr_url);
  }
  // optional config modules will use hook_alter_form to add to this array

  $config_options = array();
  //               module_name          file_name                 class_name           display_method
  $config_options['islandora_solr_search~IslandoraSolrResults.inc~IslandoraSolrResults~displayResults'] = 'Default';
  $confirmation_message = ($solr_avail ? '<img src="' . url('misc/watchdog-ok.png') . '"/>'
          . t('Successfully connected to Solr server at !islandora_solr_search_block_url', array('!islandora_solr_search_block_url' => $solr_url)) : '<img src="'
          . url('misc/watchdog-error.png') . '"/> '
          . t('Unable to connect to Solr server at !islandora_solr_search_block_url</p>', array('!islandora_solr_search_block_url' => $solr_url)));

  $form = array();

  $form['solr_ahah_wrapper'] = array(
    '#prefix' => '<div id="solr-url">',
    '#suffix' => '</div>',
    '#type' => 'fieldset',
  );

  $form['solr_ahah_wrapper']['islandora_solr_search_block_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr url'),
    '#size' => 40,
    '#weight' => -1,
    '#description' => t('The url of the Solr installation.  Defaults to localhost:8080/solr.'),
    '#default_value' => $solr_url,
    '#ahah' => array(
      'path' => 'islandora/solr/process',
      'wrapper' => 'solr-url',
      'effect' => 'fade',
      'event' => 'blur',
    ),
  );

  $form['solr_ahah_wrapper']['refresh_page'] = array(
    '#type' => 'submit',
    '#value' => t('Test connection'),
    '#attributes' => array('class' => 'refresh-button'),
    '#submit' => array('solr_admin_refresh'),
  );

  $form['solr_ahah_wrapper']['infobox'] = array(
    '#type' => 'item',
    '#value' => $confirmation_message,
  );

  $form['solr_ahah_wrapper']['islandora_solr_search_block_request_handler'] = array(
    '#type' => 'select',
    '#title' => t('Request Handler'),
    '#options' => $handlers,
    '#description' => t('Request handlers defined by !config', array('!config' => l('solrconfig.xml', 'http://wiki.apache.org/solr/SolrConfigXml'))),
    '#default_value' => $handler,
    '#ahah' => array(
      'path' => 'islandora/solr/process',
      'wrapper' => 'solr-url',
      'effect' => 'fade',
      'event' => 'change',
    ),
  );

  if ($handler == 'standard') {
    $form['solr_ahah_wrapper']['solr_namespace_restriction'] = array(
      '#type' => 'textfield',
      '#title' => t('Limit results to namespace'),
      '#size' => 5,
      '#default_value' => variable_get('solr_namespace_restriction', ''),
      '#description' => t("Enter a namespace ie 'demo' to restrict results to PIDs within that namespace <br /><em>note: this setting may only be used with 'standard' request handler.</em>"),
    );
  }
  $profiles = module_invoke_all("islandora_solr_primary_display");
  uksort($profiles, "sort_default_first");
  foreach ($profiles as $machine_name => $profile) {
    $islandora_solr_primary_display_options[$machine_name] = $profile['name'];
  }

  if (!empty($islandora_solr_primary_display_options)) {
    $form['islandora_solr_primary_display'] = array(
      '#type' => 'radios',
      '#title' => t('Default Display Profile'),
      '#options' => $islandora_solr_primary_display_options,
      '#default_value' => variable_get('islandora_solr_primary_display', 'default'),
      '#description' => "Preferred normal display profile for search results.  These may be provided by third-party modules. ",
    );
  }

  $profiles = module_invoke_all("islandora_solr_secondary_display");
  uksort($profiles, "sort_default_first");
  foreach ($profiles as $machine_name => $profile) {
    $islandora_solr_secondary_display_options[$machine_name] = $profile['name'];
  }

  if (!empty($islandora_solr_secondary_display_options)) {
    $form['islandora_solr_secondary_display'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Secondary Display Profiles'),
      '#options' => $islandora_solr_secondary_display_options,
      '#default_value' => variable_get('islandora_solr_secondary_display', array()),
      '#description' => "Enabled secondary output types. ",
    );
  }

  $form['islandora_solr_search_block_repeat'] = array(
    '#type' => 'textfield',
    '#title' => t('Search Field Repetition'),
    '#size' => 5,
    '#description' => t('The number of times you would like the search blocks to be repeated'),
    '#default_value' => variable_get('islandora_solr_search_block_repeat', '3'),
  );

  $form['islandora_solr_searchterms'] = array(
    '#type' => 'textarea',
    '#title' => t('Search Terms'),
    '#description' => t('A list of fields to populate the terms dropdown for the fielded search block. An optional friendly term my be added to each solr term for a more readable display. <br />
      Each term must be on its own line, and the solr term and the friendly term must be separated with a tilde. ie <strong>dc.title ~ Title</strong> '),
    '#default_value' => variable_get('islandora_solr_searchterms', 'dc.title ~ Title'),
    '#wysiwyg' => FALSE,
  );

  $form['islandora_solr_search_block_facets'] = array(
    '#type' => 'textarea',
    '#title' => t('Facet Fields'),
    '#description' => t('A list of fields for faceting. An optional friendly term my be added to each solr term for a more readable display. <br />
      Each term must be on its own line, and the solr term and the friendly term must be separated with a tilde. ie <strong>yearPublished ~ Date of Publication</strong>'),
    '#default_value' => variable_get('islandora_solr_search_block_facets', 'dc.subject ~ Subject,dc.type ~ Type'),
    '#wysiwyg' => FALSE,
  );

  $form['islandora_solr_search_result_fields'] = array(
    '#type' => 'textarea',
    '#title' => t('Resultset Fields'),
    '#description' => t('Similar to above, this field accepts a set of "computer_name ~ Human Readable Name" pairs.  While the list of resultset fields is typically governed by the backend request handler chosen above, this section allows you to specify human-readable substitutions for the index field names returned in a query.  Note that the substitutions specified in the Search Terms and Facet Fields sections above will also be applied to search results.'),
    '#default_value' => variable_get('islandora_solr_search_result_fields', 'dc.subject ~ Subject,dc.type ~ Type'),
    '#wysiwyg' => FALSE,
  );

  $form['islandora_solr_search_limit_result_fields'] = array(
    '#type' => 'checkbox',
    '#title' => t('Limit results to these fields?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_search_limit_result_fields', 0),
    '#description' => t('If checked, results displayed will be limited to the indicated fields.  If not, it will serve only as a substitution list for result field names.  <br/><strong>Note:</strong> Some display profiles may not honour this value.'),
  );

  $form['islandora_solr_search_block_facet_min_count'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum facet fields responses'),
    '#size' => 5,
    '#description' => t('This param indicates the minimum counts for facet fields should be included in the response. '),
    '#default_value' => variable_get('islandora_solr_search_block_facet_min_count', '2'),
  );
  $form['islandora_solr_search_block_facet_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum facet field constraints'),
    '#size' => 5,
    '#description' => t('This param indicates the maximum number of constraint counts that should be returned for the facet fields. A negative value means unlimited. '),
    '#default_value' => variable_get('islandora_solr_search_block_facet_limit', '10'),
  );
  $form['islandora_solr_search_num_of_results'] = array(
    '#type' => 'textfield',
    '#title' => t('Results per page'),
    '#size' => 5,
    '#description' => t('This param indicates the number of results to show per page. '),
    '#default_value' => variable_get('islandora_solr_search_num_of_results', '20'),
  );
  $form['islandora_solr_search_debug_mode'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug Mode?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_search_debug_mode', 0),
    '#description' => t('Dumps solr query info to the screen for testing purposes.'),
  );

  $form['buttons']['submit'] = array('#type' => 'submit', '#value' => t('Save Solr Configuration'));
  $form['buttons']['reset'] = array('#type' => 'submit', '#value' => t('Reset to defaults'));

  if (!empty($_POST) && form_get_errors()) {
    drupal_set_message(t('The settings have not been saved because of the errors.'), 'error');
  }
  $form['#submit'][] = 'solr_settings_form_submit';
  $form['#theme'] = 'system_settings_form';


  return ($form);
}

/**
 *
 * @param String $solr_url
 * @return boolean
 *
 * Checks availability of Solr installation
 *
 */
function solr_available($solr_url) {
  // path from url is parsed to allow graceful inclusion or exclusion of 'http://'
  $pathParts = parse_url($solr_url);
  $path = 'http://' . $pathParts['host'] . ':' . $pathParts['port'] . $pathParts['path'] . '/admin/file';
  $test = @fopen($path, "r");
  if ($test) {
    return true;
  }
  return false;
}

/**
 *
 * @param String $solr_url
 * @return array
 */
function get_handlers($solr_url) {
  $url = $solr_url . '/admin/file/?file=solrconfig.xml';
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  curl_setopt($ch, CURLOPT_URL, $url);
  $xml = simplexml_load_string(curl_exec($ch));
  foreach ($xml->requestHandler as $handler) {
    if ($handler['class'] == 'solr.SearchHandler') {
      $handlerName = (string) $handler['name'];
      $slashed = preg_match('/^\//', $handlerName);
      if ($handlerName != 'dismax' && $handlerName != 'partitioned' & !$slashed) {
        $handlers[$handlerName] = $handlerName;
      }
    }
  }
  return $handlers;
}

/**
 * Checks url validity and refreshes requestHandler dropdown list
 */
function update_solr_url_div() {
  $form = callback_prep();
  $changed_elements = $form['solr_ahah_wrapper'];
  unset($changed_elements['#prefix'], $changed_elements['#suffix']);
  $output = theme('status_messages') . drupal_render($changed_elements);
  drupal_json(array(
    'status' => TRUE,
    'data' => $output,
  ));
}

/**
 *
 * @return array
 * Utilty function to prep $form_state for ahah
 */
function callback_prep() {
  $form_state = array('storage' => NULL, 'submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  $form = form_get_cache($form_build_id, $form_state);
  $args = $form['#parameters'];
  $form_id = array_shift($args);
  $form_state['post'] = $form['#post'] = $_POST;
  // Enable the submit/validate handlers to determine whether AHAH-submittted.
  $form_state['ahah_submission'] = TRUE;
  $form['#programmed'] = $form['#redirect'] = FALSE;
  drupal_process_form($form_id, $form, $form_state);
  $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);
  return $form;
}

/**
 *
 * @param array $form
 * @param array $form_state
 * @return null
 */
function solr_settings_form_submit($form, &$form_state) {

  if ($form_state['clicked_button']['#id'] != 'edit-submit') {
    $form_state['rebuild'] = TRUE;
    return;
  }
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';

  // Exclude unnecessary elements.
  unset($form_state['values']['submit'], $form_state['values']['reset'], $form_state['values']['form_id'], $form_state['values']['op'], $form_state['values']['form_token'], $form_state['values']['form_build_id']);

  foreach ($form_state['values'] as $key => $value) {
    if ($op == t('Reset to defaults')) {
      variable_del($key);
    }
    else {
      if (is_array($value) && isset($form_state['values']['array_filter'])) {
        $value = array_keys(array_filter($value));
      }
      variable_set($key, $value);
    }
  }
  if ($op == t('Reset to defaults')) {
    drupal_set_message(t('The configuration options have been reset to their default values.'));
  }
  else {
    drupal_set_message(t('The solr configuration options have been saved.'));
  }

  cache_clear_all();
  drupal_rebuild_theme_registry();
}

function solr_admin_refresh($form, &$form_state) {
  $values = $form_state['values'];
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  $form_state['rebuild'] = TRUE;
}

/**
 * Function sort_default_first
 * 
 * Description: sorts two values by strcasecmp, with the special begaviour that 
 * the value "default" sorts before any other value.
 * 
 * @param <string> $a
 * @param <string> $b
 * @return <int> 
 */
function sort_default_first($a, $b) {
  if (!strcasecmp($a, 'default'))
    return -1;
  if (!strcasecmp($b, 'default'))
    return 1;
  return strcasecmp($a, $b);
}
