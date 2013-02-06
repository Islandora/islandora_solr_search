<?php

/**
 * @file
 * Contains methods to build and execute a solr query. Depends on
 * Apache_Solr_Php client.
 */

DEFINE('SOLR_SEARCH_PATH', 'islandora/search');

/**
 * Islandora Solr Query Processor.
 *
 * Used to store Solr query parameters and to connect to Solr to execute the
 * query. Populates the islandoraSolrResult property with the processed Solr
 * query results.
 */
class IslandoraSolrQueryProcessor {

  public $solrQuery;
  // Query alternative set if solrQuery is empty.
  public $internalSolrQuery;
  public $solrStart;
  public $solrLimit;
  public $solrDefType;
  // All other Solr parameters.
  public $solrParams = array();

  // Solr results tailored for Islandora's use.
  public $islandoraSolrResult;
  // the current display (for modules wanting to alter the query of a display)
  public $display;
  // parameters from url
  public $internalSolrParams;
  public $different_kinds_of_nothing = array(' ', '%20', '%252F', '%2F', '%252F-', '');

  /**
   * Build and execute a query.
   *
   * @param string $query
   *   The query string provided in the url.
   * @param type $params
   *   All url parameters from the solr results page.
   * @param boolean $alter_results
   *   Whether or not to send out hooks to alter the islandora_solr_results.
   *
   * @return NULL
   *   Nothing to return.
   */
  function buildAndExecuteQuery($query, $params = NULL, $alter_results = TRUE) {
    // set empty string
    if (variable_get('islandora_solr_request_handler', '') == 'standard') {
      if (!$query || $query == ' ') {
        $query = '%252F';
      }
    }
    // Build the query and apply admin settings.
    $this->buildQuery($query, $params);

    // Execute the query.
    $this->executeQuery($alter_results);

    return;
  }

  /**
   * Builds Solr query and performs checks based on url parameters and
   * defaults set in the Islandora Solr admin form. Populates the properties to
   * be used for the query execution. Includes a module_invoke_all to make
   * changes to the query.
   *
   * @see IslandoraSolrQueryProcessor::buildAndExecuteQuery()
   *
   * @param type $query
   *   The query string provided in the url.
   * @param array $params
   *   All URL parameters from the solr results page.
   */
  function buildQuery($query, $params = array() ) {
    // Set internal parameters gathered from the URL but not q and page.
    $this->internalSolrParams = $params;
    unset($this->internalSolrParams['q']);
    unset($this->internalSolrParams['page']);

    // Set Solr type (dismax)
    if (isset($this->internalSolrParams['type']) && ($this->internalSolrParams['type'] == 'dismax' || $this->internalSolrParams['type'] == 'edismax')) {
      $this->solrDefType = $this->internalSolrParams['type'];
      $this->solrParams['defType'] = $this->internalSolrParams['type'];
    }

    // Set query
    // fix the query as some characters will break the search : and / slash are examples
    $this->solrQuery = urldecode(restore_slashes($query));

    // if the query is empty
    if (empty($this->solrQuery) || in_array($this->solrQuery, $this->different_kinds_of_nothing)) {
      $this->solrQuery = ' '; //so we can allow empty queries to dismax

      // set base query
      $this->internalSolrQuery = variable_get('islandora_solr_base_query', 'timestamp:[* TO NOW]');
      // we must also undo dismax if it's been set
      $this->solrDefType = NULL;
      $this->solrParams['defType'] = NULL;
    }

    // set sort
    if (isset($this->internalSolrParams['sort'])) {
      $sort_explode = explode(' ', $this->internalSolrParams['sort']);
      // check if an order is given and if the order value is asc or desc
      if (isset($sort_explode[1]) && ($sort_explode[1] == 'asc' || $sort_explode[1] == 'desc')) {
        $this->solrParams['sort'] = $this->internalSolrParams['sort'];
      }
      else {
        // use ascending
        $this->solrParams['sort'] = $sort_explode[0] . ' asc';
      }
    }

    // set display property (so display plugin modules can use this in a conditional to alter the query class)
    if (isset($this->internalSolrParams['display'])) {
      $this->display = $this->internalSolrParams['display'];
    }
    else {
      $this->display = variable_get('islandora_solr_primary_display', 'default');
    }

    // get pager variable
    $startPage = isset($_GET['page']) ? $_GET['page'] : 0;

    // set results limit
    $this->solrLimit = isset($this->internalSolrParams['limit']) ? $this->internalSolrParams['limit'] : variable_get('islandora_solr_num_of_results', 20);

    // set solr start
    $this->solrStart = max(0, $startPage) * $this->solrLimit;

    // set facet parameters
    // set variables
    $facet_array = islandora_solr_get_fields('facet_fields');
    $facet_fields = implode(",", array_keys($facet_array));
    $keys = array();
    $snippet_array = islandora_solr_get_snippet_fields();
    $snippets = implode(',', $snippet_array);

    // set params
    $params_array = array(
      'facet' => 'true',
      'facet.mincount' => variable_get('islandora_solr_facet_min_limit', '2'),
      'facet.limit' => variable_get('islandora_solr_facet_max_limit', '20'),
      'qt' => variable_get('islandora_solr_request_handler', ''),
      'hl' => isset($snippets) ? 'true' : NULL,
      'hl.fl' => isset($snippets) ? $snippets : NULL,
      'hl.fragsize' => 400,
        // Comma separated list configured in the block config.
      'facet.field' => explode(',', $facet_fields),
    );

    // check for date facets
    $facet_dates = islandora_solr_get_range_facets();
    if (!empty($facet_dates)) {
      // set range/date variables
      $params_date_facets = array();
      foreach ($facet_dates as $key => $value) {
        $field = $value['solr_field'];
        $start = $value['solr_field_settings']['range_facet_start'];
        $end = $value['solr_field_settings']['range_facet_end'];
        $gap = $value['solr_field_settings']['range_facet_gap'];
        // add date facet
        $params_date_facets["facet.date"][] = $field;
        // custom field settings
        if ($start) {
          $params_date_facets["f.{$field}.facet.date.start"] = $start;
        }
        if ($end) {
          $params_date_facets["f.{$field}.facet.date.end"] = $end;
        }
        if ($gap) {
          $params_date_facets["f.{$field}.facet.date.gap"] = $gap;
        }
        // remove range/date field from facet.field array
        $pos = array_search($field, $params_array['facet.field']);
        unset($params_array['facet.field'][$pos]);
      }
      // default settings
      $params_date_facets["facet.date.start"] = 'NOW/YEAR-20YEARS';
      $params_date_facets["facet.date.end"] = 'NOW';
      $params_date_facets["facet.date.gap"] = '+1YEAR';

      $params_array = array_merge($params_array, $params_date_facets);
    }

    // add parameters
    $this->solrParams = array_merge($this->solrParams, $params_array);

    // Set base filters.
    $base_filters = preg_split("/\\r\\n|\\n|\\r/", variable_get('islandora_solr_base_filter', ''), -1, PREG_SPLIT_NO_EMPTY);

    // Set filter parameters - both from url and admin settings.
    if (isset($this->internalSolrParams['f']) && is_array($this->internalSolrParams['f'])) {
      $this->solrParams['fq'] = $this->internalSolrParams['f'];
      if (!empty($base_filters)) {
        $this->solrParams['fq'] = array_merge($this->internalSolrParams['f'], $base_filters);
      }
    }
    elseif (!empty($base_filters)) {
      $this->solrParams['fq'] = $base_filters;
    }

    // Restrict results based on specified namespaces.
    $namespace_list = trim(variable_get('islandora_solr_namespace_restriction', ''));
    if ($namespace_list) {
      $namespaces = preg_split('/[,|\s]/', $namespace_list);
      $namespace_array = array();
      foreach (array_filter($namespaces) as $namespace) {
        $namespace_array[] = "PID:$namespace\:*";
      }
      $this->solrParams['fq'][] = implode(' OR ', $namespace_array);
    }

    // If no qf fields are specified in the requestHandler a default list is
    // supplied here for dismax searches.
    if (!variable_get('islandora_solr_dismax_allowed', FALSE) && isset($this->internalSolrParams['type']) && ($this->internalSolrParams['type'] == "dismax" || $this->internalSolrParams['type'] == "edismax")) {
      $this->solrParams['qf'] = 'dc.title^5 dc.subject^2 dc.description^2 dc.creator^2 dc.contributor^1 dc.type';
    }

    // Invoke a hook for third-party modules to alter the parameters.
    // The hook implementation needs to specify that it takes a reference.
    module_invoke_all('islandora_solr_query', $this);
  }

  /**
   * Reset Results
   *
   * @return NULL
   *   Nothing to return.
   */
  function resetResults() {
    unset($this->islandoraSolrResult);
    return;
  }

  /**
   * Connects to Solr and executes the query. Populates islandoraSolrResults
   * property with the raw Solr results.
   *
   * @param boolean $alter_results
   *   Whether or not to send out hooks to alter the islandora_solr_results.
   *
   * @return NULL
   *   Nothing to return.
   */
  function executeQuery($alter_results = TRUE) {
    // Init Apache_Solr_Service object.
    $path_parts = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
    $solr = new Apache_Solr_Service($path_parts['host'], $path_parts['port'], $path_parts['path'] . '/');
    $solr->setCreateDocuments(0);

    // Query is executed.
    try {
      $solr_query = ($this->internalSolrQuery) ? $this->internalSolrQuery : $this->solrQuery;
      $results = $solr->search($solr_query, $this->solrStart, $this->solrLimit, $this->solrParams);
    }
    catch (Exception $e) {
      drupal_set_message(check_plain(t('Error searching Solr index')) . ' ' . $e->getMessage(), 'error');
    }

    if (isset($results)) {
      $solr_results = json_decode($results->getRawResponse(), TRUE);
      // Create results tailored for Islandora's use.
      $object_results = $solr_results['response']['docs'];
      $datastream_field = variable_get('islandora_solr_datastream_id_field', 'fedora_datastreams_ms');
      foreach ($object_results as $object_index => $object_result) {
        unset($object_results[$object_index]);
        $object_results[$object_index]['solr_doc'] = $object_result;
        $object_results[$object_index]['object_url'] = 'islandora/object/' . $object_results[$object_index]['solr_doc']['PID'];
        $object_results[$object_index]['object_url_params'] = array();

        // Set thumbnail to TN datastream if there is no $datastream_field or
        // if there is a populated TN datastream.
        if (!isset($object_result[$datastream_field]) || in_array('TN', $object_result[$datastream_field])) {
          $object_results[$object_index]['thumbnail_url'] = $object_results[$object_index]['object_url'] . '/datastream/TN/view';
        }
        else {
          $object_results[$object_index]['thumbnail_url'] = drupal_get_path('module', 'islandora_solr') . '/images/defaultimg.png';
        }
        $object_results[$object_index]['thumbnail_url_params'] = array();
      }
      // Allow other parts of code to modify the tailored results.
      if ($alter_results) {
        // Hook to alter based on content model.
        module_load_include('inc', 'islandora', 'includes/utilities');
        $content_model_solr_field = variable_get('islandora_solr_content_model_field', 'RELS_EXT_hasModel_uri_ms');
        foreach ($object_results as $object_index => $object_result) {
          foreach ($object_result['solr_doc'][$content_model_solr_field] as $content_model_uri) {
            $hook_list = islandora_build_hook_list('islandora_solr_object_result', array(substr($content_model_uri, 12)));
            drupal_alter($hook_list, $object_results[$object_index], $this);
          }
        }
        // Hook to alter everything.
        drupal_alter('islandora_solr_results', $object_results, $this);
      }
      // Save results tailored for Islandora's use.
      unset($solr_results['response']['docs']);
      $solr_results['response']['objects'] = $object_results;
      $this->islandoraSolrResult = $solr_results;
    }
    else {
      $this->islandoraSolrResult = NULL;
    }
    return;
  }
}
