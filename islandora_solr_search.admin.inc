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
  $dismax_allowed = check_dismax($solr_url, $handler);
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
    '#description' => t('Request handlers as defined by !config', array('!config' => l('solrconfig.xml', 'http://wiki.apache.org/solr/SolrConfigXml'))),
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
      '#description' => "Enabled secondary output/download types for search results.",
    );
  }

  //Notice: Undefined index: #options in expand_checkboxes() (line 1930 of /Applications/MAMP/htdocs/drupal-6.22/includes/form.inc).
  $form['dismax_allowed'] = array(
    '#type' => 'hidden',
    '#value' => $dismax_allowed,
  );

  $form['islandora_solr_search_block_repeat'] = array(
    '#type' => 'textfield',
    '#title' => t('Search Field Repetition'),
    '#size' => 5,
    '#description' => t('The number of fields you would like to have appear in the <strong>Solr Advanced Search Block</strong>'),
    '#default_value' => variable_get('islandora_solr_search_block_repeat', '3'),
  );

  $form['islandora_solr_searchterms'] = array(
    '#type' => 'textarea',
    '#title' => t('Search Terms'),
    '#description' => t("Indicate what fields should appear in the dropdown menu of terms for the <strong>Solr Advanced Search Block</strong>.<br />
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.title ~ Title</strong><br />
      Review the schema.xml to see what terms are available. A list of the fields available when using the schema.xml packaged with the Islandora Solr
      module is available in Islandora's online documentation."),
    '#default_value' => variable_get('islandora_solr_searchterms', 'dc.title ~ Title'),
    '#wysiwyg' => FALSE,
  );
  $form['islandora_solr_snippet_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Snippet Field'),
    '#default_value' => variable_get('islandora_solr_snippet_field', ''),
    '#description' => t("If a match is found on this field, a snippet of text will be returned, with the search term highlighted.<br />
      An optional friendly label may inserted using the following pattern <strong>dsm.Text ~ Full Text</strong><br />
      <strong>Note:</strong><em> This feature is not supported by all display profiles.</em> "),
    '#wysiwyg' => FALSE,
  );
  $form['islandora_solr_search_block_facets'] = array(
    '#type' => 'textarea',
    '#title' => t('Facet Fields'),
    '#description' => t("Indicate which fields will appear in the <strong>Islandora Facet Block</strong>.<br />
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.title ~ Title</strong> "),
    '#wysiwyg' => FALSE,
  );

  $form['islandora_solr_search_result_fields'] = array(
    '#type' => 'textarea',
    '#title' => t('Labels for Returned Fields'),
    '#description' => t('Set labels for fields returned from query.
      Enter terms on separate lines using the following pattern: field [tilde] preferred label. ie <strong>dc.title ~ Title</strong>'),
    '#default_value' => variable_get('islandora_solr_search_result_fields', 'dc.subject ~ Subject,dc.type ~ Type'),
    '#wysiwyg' => FALSE,
  );

  $form['islandora_solr_search_limit_result_fields'] = array(
    '#type' => 'checkbox',
    '#title' => t('Limit results to fields listed above?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_search_limit_result_fields', 0),
    '#description' => t('If checked, results displayed will be limited to the fields specified above.<br/><strong>Note:</strong> Some display profiles may not honour this value.'),
  );

  $form['islandora_solr_search_block_facet_min_count'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum Results to Display Facet'),
    '#size' => 5,
    '#description' => t('Minimum facet count required for display'),
    '#default_value' => variable_get('islandora_solr_search_block_facet_min_count', '2'),
  );
  $form['islandora_solr_search_block_facet_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum Facets returned'),
    '#size' => 5,
    '#description' => t('Set the number of terms that should be returned to the user.<br />
      For example, if there are 100 possible subjects in a faceted result, you may wish to return only the top 10.'),
    '#default_value' => variable_get('islandora_solr_search_block_facet_limit', '10'),
  );
  $form['islandora_solr_search_num_of_results'] = array(
    '#type' => 'textfield',
    '#title' => t('Results per page'),
    '#size' => 5,
    '#description' => t('The number of results to show per page. '),
    '#default_value' => variable_get('islandora_solr_search_num_of_results', '20'),
  );
  $form['islandora_solr_search_debug_mode'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug Mode?'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_solr_search_debug_mode', 0),
    '#description' => t('Dumps solr query to the screen for testing'),
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
    return TRUE;
  }
  return FALSE;
}

/**
 * get handlers??
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

function check_dismax($solr_url, $selected_handler) {
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
              $dismax= TRUE;
              $dismax = true;
            }
          }
        }
      }
    }
  }
  return $dismax;
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
 * callback prep ??
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
 * Solr settings from submin
 * @param array $form
 * @param array $form_state
 * @return NULL
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

/**
 * Solr admin refresh
 * @param type $form
 * @param array $form_state 
 */
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
