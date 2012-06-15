<?php

/**
 * @file
 * Contains methods to build and execute a solr query.
 * Depends on Apache_Solr_Php client.
 */

DEFINE('SOLR_SEARCH_PATH','islandora/solr/search');
DEFINE('SOLR_BASE_QUERY','timestamp:[* TO NOW]');

/**
 * Islandora Solr Query Processor
 * @todo Stop using global vars, start using this object's vars.
 *  The module itself should initially create an instance of this
 *  object and reference the query vars inside that object when calling
 *  other output display functions.
 */
class IslandoraSolrQueryProcessor {

  static $facetSeparator = '~'; //used to separate facets in url
  static $slashReplacement = '~slsh~'; // a pattern used to replace / in url's the slash breaks drupal clean url's
  public static $SEARCH_CLASS_ADVANCED_SEARCH_NUMBER_FIELDS = 5;
  
  public $solrQuery;
  public $internalSolrQuery; // query alternative set if solrQuery is empty
  public $solrParams = array();
  public $internalSolrParams; // parameters from url
  public $solrStart;
  public $solrLimit;
  public $solrDefType;

  public $solrResult;

  public $display;
  public $different_kinds_of_nothing = array( ' ', '%20', '%252F', '%2F', '%252F-', ''); 

  /**
   * Constructor
   */
  function IslandoraSolrQueryProcessor() {
    if (module_exists('apachesolr')) {
      module_load_include('php', 'apachesolr', 'SolrPhpClient/Apache/Solr/Service');
    }
    else {
      module_load_include('php', 'islandora_solr_search', 'SolrPhpClient/Apache/Solr/Service');
    }
    module_load_include('inc', 'islandora_solr_search', 'includes/common');
  }

  /**
   * Build and execute a query
   * @param type $query
   * @param type $params
   * @return type
   */
  function buildAndExecuteQuery($query, $params = NULL) {
    if (variable_get('islandora_solr_search_block_request_handler', NULL) == 'standard') {

      if (!$query || $query == ' ') {
        $query = '%252F';
      }
    }
    $this->buildQuery($query, $params);

    return $this->executeQuery();
  }

  /**
   * Default implementation for solr search.  You can configure others in the block config settings
   */
  function buildQuery($query, $params = array() ) {
    
    // set internal parameters gathered from the url
    $this->internalSolrParams = $params;
    
    // don't store the q and page parameter internally
    unset($this->internalSolrParams['q']);
    unset($this->internalSolrParams['page']);
    
    // Set Solr type (dismax)
    if ($this->internalSolrParams['type'] == 'dismax') {
      $this->solrDefType = $this->internalSolrParams['type'];
      $this->solrParams['defType'] = $this->internalSolrParams['type'];
    }

    // Set query
    // fix the query as some characters will break the search : and / slash are examples
    $this->solrQuery = urldecode(restoreSlashes($query));

    // if the query is empty
    if (empty($this->solrQuery) || in_array($this->solrQuery, $this->different_kinds_of_nothing)) {
      $this->solrQuery = ' '; //so we can allow empty queries to dismax
      
      // set base query
      $this->internalSolrQuery = variable_get('islandora_solr_search_base_query', SOLR_BASE_QUERY);
      // we must also undo dismax if it's been set
      $this->solrDefType = NULL;
      $this->solrParams['defType'] = NULL;
    }
    
    // Set display variable
    if (isset($this->internalSolrParams['display'])) {
      $display = $this->internalSolrParams['display'];
      $this->display = $display;
      // if current display is default, unset display key.
      if ($display == variable_get('islandora_solr_primary_display', 'default') ) {
        unset($this->internalSolrParams['display']);
      }
    }
    else {
      $display = variable_get('islandora_solr_primary_display', 'default');
      $this->display = $display;
      unset($this->internalSolrParams['display']);
    }
   
    // display profiles can contain parameters. // @TODO: document that this is possible.
    $profiles = module_invoke_all("islandora_solr_primary_display");
    $profile = $profiles[$display];
    $profile_params = array();
    // if display has parameters defined
    if ($profile['params']) {
      // loop over defined paramters
      foreach ($profile['params'] as $param) {
        $profile_params[] = $param;
      }
      // if the display has parameters defined, add them to the solr parameters array
      if (is_array($profile_params)) {
        $this->solrParams = array_merge($this->solrParams, $profile_params);
      }
    }
    
    // get pager variable
    $startPage = isset($_GET['page']) ? $_GET['page'] : 0;
    
    // set results limit
    $this->solrLimit = variable_get('islandora_solr_search_num_of_results', 20);

    // set solr start
    $this->solrStart = max(0, $startPage) * $this->solrLimit;

    
    // Include the file for the display profile // @TODO: figure out why
    require_once(drupal_get_path('module', $profile['module']) . '/' . $profile['file']);

    // Set display class and function vars // @TODO: these two variables aren't used anywhere. Remove them?
    $solrClass = $profile['class'];
    $solrFunction = $profile['function'];


    
    // set facet parameters
    // set variables
    $raw_facet_vals = variable_get("islandora_solr_search_block_facets", 'dc.subject ~ Subject,dc.type ~ Type');
    $facet_array = islandora_build_substitution_list($raw_facet_vals);
    $facet_fields = implode(",", array_keys($facet_array));
    $keys = array();
    $snippet_array = islandora_build_substitution_list(variable_get("islandora_solr_snippet_field", ''));
    if (is_array($snippet_array)) {
      $keys = array_keys($snippet_array);
    }
    // set params
    $params_array = array(
      'facet' => 'true',
      'facet.field' => explode(',', $facet_fields), //comma separated list configured in the block config
      'facet.mincount' => variable_get('islandora_solr_search_block_facet_min_count', '2'),
      'facet.limit' => variable_get('islandora_solr_search_block_facet_limit', '12'),
      'qt' => variable_get("islandora_solr_search_block_request_handler", "standard"),
      'hl' => isset($keys[0]) ? 'true' : NULL,
      'hl.fl' => isset($keys[0]) ? trim($keys[0]) : NULL,
      'hl.fragsize' => 400,
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
    $debugQuery = (variable_get('islandora_solr_search_debug_mode', 0) ? "TRUE" : NULL ); //any val incl. 'FALSE' is treated as TRUE by Solr // @TODO: this variable isn't used anywhere - remove?

    // set base filters
    $base_filters = preg_split("/\\r\\n|\\n|\\r/", variable_get('islandora_solr_search_base_filter', ''), -1, PREG_SPLIT_NO_EMPTY);
    
    // set filter parameters - both from url and admin settings.
    if (is_array($this->internalSolrParams['f'])) {
      $this->solrParams['fq'] = $this->internalSolrParams['f'];
      if (!empty($base_filters)) {
        $this->solrParams['fq'] = array_merge($this->internalSolrParams['f'], $base_filters);
      }
    }
    elseif (!empty($base_filters)) {
      $this->solrParams['fq'] = $base_filters;
    }
    
    
    // restrict results based on specified namespace
    $namespace = trim(variable_get('islandora_solr_search_namespace_restriction', NULL));
    if ($namespace) {
      $this->solrParams['fq'][] = "PID:$namespace\:*";
    }

    // if no qf fields are specified in rthe requestHandler a default list is supplied here for dismax searches
    if (!variable_get('dismax_allowed', 0) && $this->internalSolrParams['type'] == "dismax") {
      $this->solrParams['qf'] = 'dc.title^5 dc.subject^2 dc.description^2 dc.creator^2 dc.contributor^1 dc.type';
    }

    // Invoke a hook for third-party modules to alter the parameters.
    module_invoke_all('islandora_solr_search_query_processor', $this); //The hook implementation needs to specify that it takes a reference, not be passed one
    $processor->solrParams['fq'][] = $fq; // @TODO figure out where $processor is set - what does it do?
    return;
  }

  /**
   * Reset Results
   */
  function resetResults() {
    unset($this->solrResult);
  }

  /**
   * Execute the query
   * @return type
   */
  function executeQuery() {
    
    // @TODO: add possibility to alter the query right before it gets excecuted.

    // set variables
    $url = variable_get('islandora_solr_search_block_url', 'http://localhost:8080/solr');
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
      drupal_set_message(check_plain(t('error searching')) . ' ' . $e->getMessage());
    }
    // save results in class property
    $this->solrResult = $results;
    // set error
    if (empty($results)) {
      drupal_set_message(t('Error searching solr index. Is the solr search block configured properly?'), 'error');
      return;
    }

    // unset variables
    unset($results);
    unset($solr);

    return;
  }

}

