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
 * Used to store Solr query parameters and to connect to solr to execute the
 * query. Populates the solrResult property with the raw Solr query results.
 */
class IslandoraSolrQueryProcessor {

  public $solrQuery; // solr query
  public $internalSolrQuery; // query alternative set if solrQuery is empty
  public $solrStart;
  public $solrLimit;
  public $solrDefType;
  public $solrParams = array(); // all other solr parameters

  public $solrResult; // raw solr results returned after query execution

  public $display; // the current display (for modules wanting to alter the query of a display)
  public $internalSolrParams; // parameters from url
  public $different_kinds_of_nothing = array( ' ', '%20', '%252F', '%2F', '%252F-', '');

  /**
   * Constructor
   */
  function IslandoraSolrQueryProcessor() {

    // include common.inc
    module_load_include('inc', 'islandora_solr', 'includes/common');
  }

  /**
   * Build and execute a query.
   *
   * @param string $query
   *   The query string provided in the url.
   * @param type $params
   *   All url parameters from the solr results page.
   * @return
   *   The contents of the executeQuery() method. ExecuteQuery() stores the raw
   *   solr results in the solrResult property.
   */
  function buildAndExecuteQuery($query, $params = NULL) {
    // include IslandoraSolrResults
    module_load_include('inc', 'islandora_solr', 'IslandoraSolrResults');

    // set empty string
    if (variable_get('islandora_solr_request_handler', '') == 'standard') {

      if (!$query || $query == ' ') {
        $query = '%252F';
      }
    }
    // build the query and apply admin settings
    $this->buildQuery($query, $params);

    // execute the query
    return $this->executeQuery();
  }

  /**
   * Builds Solr query and performs checks based on url parameters and
   * defaults set in the Islandora Solr admin form. Populates the properties to
   * be used for the query execution.
   *
   * @param type $query
   *   The query string provided in the url.
   * @param type $params
   *   All url parameters from the solr results page.
   * @return
   *   Doesn't return anything.
   *
   * @see IslandoraSolrQueryProcessor::buildAndExecuteQuery()
   */
  function buildQuery($query, $params = array() ) {

    // set internal parameters gathered from the url
    $this->internalSolrParams = $params;

    // don't store the q and page parameter internally
    unset($this->internalSolrParams['q']);
    unset($this->internalSolrParams['page']);

    // Set Solr type (dismax)
    if (isset($this->internalSolrParams['type']) AND $this->internalSolrParams['type'] == 'dismax') {
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
      if (isset($sort_explode[1]) AND ($sort_explode[1] == 'asc' OR $sort_explode[1] == 'desc')) {
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
    $raw_facet_vals = variable_get("islandora_solr_facets", 'dc.subject ~ Subject,dc.type ~ Type');
    $facet_array = islandora_build_substitution_list($raw_facet_vals);
    $facet_fields = implode(",", array_keys($facet_array));
    $keys = array();
    $snippet_array = islandora_build_substitution_list(variable_get('islandora_solr_snippet_field', ''));
    if (is_array($snippet_array)) {
      $keys = array_keys($snippet_array);
    }
    // set params
    $params_array = array(
      'facet' => 'true',
      'facet.mincount' => variable_get('islandora_solr_facet_min_limit', '2'),
      'facet.limit' => variable_get('islandora_solr_facet_max_limit', '20'),
      'qt' => variable_get('islandora_solr_request_handler', ''),
      'hl' => isset($keys[0]) ? 'true' : NULL,
      'hl.fl' => isset($keys[0]) ? trim($keys[0]) : NULL,
      'hl.fragsize' => 400,
      'facet.field' => explode(',', $facet_fields), //comma separated list configured in the block config
    );

    // check for date facets
    if (variable_get('islandora_solr_facet_date' , NULL) != NULL) {
      // set variables
      $facet_date = variable_get('islandora_solr_facet_date' , NULL);
      $facet_date = islandora_build_substitution_list($facet_date);
      $facet_date = implode(",", array_keys($facet_date));
      $params_date_facets = array(
        'facet.date' => explode(',', $facet_date),
        'facet.date.start' => variable_get('islandora_solr_facet_date_start' , 'NOW/YEAR-20YEARS'),
        'facet.date.end' => variable_get('islandora_solr_facet_date_end' , 'NOW'),
        'facet.date.gap' => variable_get('islandora_solr_facet_date_gap' , '+1YEAR'),
      );
      $params_array = array_merge($params_array, $params_date_facets);
    }

    // add parameters
    $this->solrParams = array_merge($this->solrParams, $params_array);

    // set debug query
    $debugQuery = (variable_get('islandora_solr_debug_mode', 0) ? "TRUE" : NULL ); //any val incl. 'FALSE' is treated as TRUE by Solr // @TODO: this variable isn't used anywhere - remove?

    // set base filters
    $base_filters = preg_split("/\\r\\n|\\n|\\r/", variable_get('islandora_solr_base_filter', ''), -1, PREG_SPLIT_NO_EMPTY);

    // set filter parameters - both from url and admin settings.
    if (isset($this->internalSolrParams['f']) AND is_array($this->internalSolrParams['f'])) {
      $this->solrParams['fq'] = $this->internalSolrParams['f'];
      if (!empty($base_filters)) {
        $this->solrParams['fq'] = array_merge($this->internalSolrParams['f'], $base_filters);
      }
    }
    elseif (!empty($base_filters)) {
      $this->solrParams['fq'] = $base_filters;
    }


    // restrict results based on specified namespace
    $namespace = trim(variable_get('islandora_solr_namespace_restriction', ''));
    if ($namespace) {
      $this->solrParams['fq'][] = "PID:$namespace\:*";
    }

    // if no qf fields are specified in the requestHandler a default list is supplied here for dismax searches
    if (!variable_get('islandora_solr_dismax_allowed', FALSE) AND isset($this->internalSolrParams['type']) AND $this->internalSolrParams['type'] == "dismax") {
      $this->solrParams['qf'] = 'dc.title^5 dc.subject^2 dc.description^2 dc.creator^2 dc.contributor^1 dc.type';
    }

    return;
  }

  /**
   * Reset Results
   */
  function resetResults() {
    unset($this->solrResult);
  }

  /**
   * Connects to Solr and executes the query. Collects the raw Solr results and
   * populates the solrResults property. Includes a module_invoke_all to make
   * changes to the query object before it executes.
   *
   * @return
   *   Doesn't return anything, but does populate the solrResults property with
   *   the raw Solr results.
   */
  function executeQuery() {

    // Invoke a hook for third-party modules to alter the parameters.
    //The hook implementation needs to specify that it takes a reference, not be passed one
    module_invoke_all('islandora_solr_query', $this);

    // set variables
    $url = variable_get('islandora_solr_url', 'localhost:8080/solr');
    $pathParts = parse_url($url);

    // call solr
    $solr = new Apache_Solr_Service($pathParts['host'], $pathParts['port'], $pathParts['path'] . '/');
    $solr->setCreateDocuments(0);

    // This is where the query gets executed and output starts being created.
    try {
      // solrQuery
      $solr_query = ($this->internalSolrQuery) ? $this->internalSolrQuery : $this->solrQuery;
      // execute query
      $results = $solr->search($solr_query, $this->solrStart, $this->solrLimit, $this->solrParams);
    } catch (Exception $e) {
      drupal_set_message(check_plain(t('Error searching solr index')) . ' ' . $e->getMessage(), 'error');
    }
    // save results in class property
    if (isset($results)) {
      $this->solrResult = $results;
    }
    else {
      $this->solrResult = NULL;
    }

    // unset variables
    unset($results);
    unset($solr);

    return;
  }

}

