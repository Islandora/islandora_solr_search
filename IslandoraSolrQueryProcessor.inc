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
  public $internalSolrQuery;
  public $solrParams;
  public $solrStart;
  public $solrLimit;
  public $solrDefType;
  public $solrFilters;
  public $solrResult;

  public $different_kinds_of_nothing = array( ' ', '%20', '%252F', '%2F', '%252F-', '' );

  /**
   * Constructor
   */
  function IslandoraSolrQueryProcessor() {
    module_load_include('php', 'islandora_solr_search', 'SolrPhpClient/Apache/Solr/Service');
    module_load_include('inc', 'islandora_solr_search', 'includes/common');
  }

  /**
   * Build and execute a query
   * @param type $query
   * @param type $fq
   * @param type $dismax
   * @return type
   */
  function buildAndExecuteQuery($query, $fq=NULL, $dismax=NULL, $display = NULL, $params = NULL) {
    if (variable_get('islandora_solr_search_block_request_handler', NULL) == 'standard') {

      if (!$query || $query == ' ') {
        $query = '%252F';
      }
    }
    $this->buildQuery($query, $fq, $dismax, $display, $params);

    return $this->executeQuery();
  }

  /**
   * Default implementation for solr search.  You can configure others in the block config settings
   * @todo Get rid of the global $facetArray?  It is the only global yet relied upon.
   */
  function buildQuery($query, $fq = NULL, $dismax = NULL, $display = NULL, $params = array() ) {
    // Get pager variable
    $startPage = isset($_GET['page']) ? $_GET['page'] : 0;
    $this->solrLimit = variable_get('islandora_solr_search_num_of_results', 20);
    global $base_url, $facetArray; //used by facet block as well
    // -- cut here --
    // Get display profile
    $islandora_solr_primary_display = variable_get('islandora_solr_primary_display', 'default');
    $profiles = module_invoke_all("islandora_solr_primary_display");
    if (!isset($profiles[$islandora_solr_primary_display])) {
      drupal_set_message(t('There is an error in the solr search configuration: the display profile is not found.'), 'error');
      return "";
    }
    $profile = $profiles[$islandora_solr_primary_display];
    if ($profile['params']) {
      foreach ($profile['params'] as $param) {
        $params[] = $param;
      }
    }
    
    if ($display == variable_get('islandora_solr_primary_display', 'default')) {
      $display_param = '';
    }
    else {
      $display_param = $display;
    }
    
    // Include the file for the display profile
    require_once(drupal_get_path('module', $profile['module']) . '/' . $profile['file']);

    // Set display class and function vars
    $solrClass = $profile['class'];
    $solrFunction = $profile['function'];

    // -- end cut here --
    //fix the query as some characters will break the search : and / slash are examples
    $this->solrQuery = urldecode(restoreSlashes($query));

    if (empty($this->solrQuery) || in_array($this->solrQuery,$this->different_kinds_of_nothing)) {
      $this->solrQuery = ' '; //so we can allow empty queries to dismax
    }

    $facetArray = array();
    $facetFields = '';
    $rawFacetVals = variable_get("islandora_solr_search_block_facets", 'dc.subject,dc.type');
    $facetArray = islandora_build_substitution_list($rawFacetVals);
    $facetFields = implode(",", array_keys($facetArray));
    $keys = array('');
    $snippetArray = islandora_build_substitution_list(variable_get("islandora_solr_snippet_field", ""));
    if (is_array($snippetArray)) {
      $keys = array_keys($snippetArray);
    }

    $facetlimit = variable_get('islandora_solr_search_block_facet_limit', '12');
    $facetMinCount = variable_get('islandora_solr_search_block_facet_min_count', '2');
    $requestHandler = variable_get("islandora_solr_search_block_request_handler", "standard");
    $highlight_field = variable_get("islandora_solr_snippet_field", "");
    $this->solrParams = array(
      'facet' => 'true',
      'facet.mincount' => $facetMinCount,
      'facet.limit' => $facetlimit,
      'qt' => $requestHandler,
      'hl' => isset($keys[0]) ? 'true' : NULL,
      'hl.fl' => isset($keys[0]) ? trim($keys[0]) : NULL,
      'hl.fragsize' => 400,
      'facet.field' => explode(',', $facetFields), //comma separated list configured in the block config
    );


    if (is_array($params)) {
      $this->solrParams = array_merge($this->solrParams, $params);
    }
    $debugQuery = (variable_get('islandora_solr_search_debug_mode', 0) ? "TRUE" : NULL ); //any val incl. 'FALSE' is treated as TRUE by Solr

    $base_filters = preg_split("/\\r\\n|\\n|\\r/", variable_get('islandora_solr_search_base_filter', ''), -1, PREG_SPLIT_NO_EMPTY);

    if ($fq != NULL && $fq != '-') {
      $fq = restoreSlashes($fq); //put the slash back

      $fqs = csv_explode(IslandoraSolrQueryProcessor::$facetSeparator, $fq, '"', TRUE); //to filter by more then one facet we will separate them by~ for nowseparate them by~ for now
      $fqs = array_merge($fqs,$base_filters);
      $this->solrParams['fq'] = $fqs;
      $islandora_fq = replaceSlashes($fq); //remove the slash here as we will be using this in url's
    } elseif (!empty($base_filters)) {
      $this->solrParams['fq'] = $base_filters;
    }
    if (empty($islandora_fq)) {
      $islandora_fq = '-';
    }
    if ($dismax != NULL) {
      $this->solrDefType = $dismax;
      $this->solrParams['defType'] = $dismax;
    }
    $this->solrStart = max(0, $startPage) * $this->solrLimit;

    // The breadcrumb should go in the display class
    $queryurl = SOLR_SEARCH_PATH . "/" . replaceSlashes($this->solrQuery);
    $breadcrumb_fq = $islandora_fq;
    $facet = NULL; // Nigelb, This is being used outside of the for loop where it is being created this is probably incorrect.
    if (strcmp($islandora_fq, "-")) {
      foreach (csv_explode(IslandoraSolrQueryProcessor::$facetSeparator, $islandora_fq, '"', TRUE) as $facet) {
        $field = drupal_substr($facet, 0, strpos($facet, ":"));
        $value = restoreSlashes(trim(drupal_substr($facet, strpos($facet, ":") + 1), '"'));
        $options = array('html' => TRUE);
        $options['attributes']['title'] = $facet;
        $crumblink = $queryurl . "/" . $breadcrumb_fq . (empty($this->solrDefType) ? '/-' : '/' . $this->solrDefType) . '/' . $display_param;
        $cut_fq = $this->delete_filter($islandora_fq, $facet);
        $cutlink = $queryurl . "/" . $cut_fq . (empty($this->solrDefType) ? '/-' : '/' . $this->solrDefType) . '/' . $display_param;
        if (!strncmp("-", $facet, 1)) {
          $options['attributes']['class'] = "strikethrough";
        }
        $breadcrumb[] = l($value, $crumblink, $options) .
            " <span class='islandora_solr_search_breadcrumb_super'>(" . l(t('x'), $cutlink, array('attributes' => array('title' => "Remove " . $facet))) . ")</span>";
        $breadcrumb_fq = $this->delete_filter($breadcrumb_fq, $facet);
      }
    }
    if (!in_array($this->solrQuery,$this->different_kinds_of_nothing)) {
      $cutlink = SOLR_SEARCH_PATH . "/ /" . $islandora_fq . '/-/' . $display_param;
      $queryval = $this->solrQuery;
      $tokens = explode(' ', $queryval);
      foreach ($tokens as $token) {
        preg_match("/^.*:/", $token, $matches);
        if (isset($matches[0])) {
          $queryval = str_replace($matches[0], "", $queryval);
        }
      }
      if ($queryval != '%252F') {
        $queryurl = "$queryurl/-/{$this->solrDefType}/$display_param";
        $breadcrumb[] = l($queryval, $queryurl, array('attributes' => array('title' => $this->solrQuery))) .
            " <span class='islandora_solr_search_breadcrumb_super'>(" . l(t('x'), $cutlink, array('attributes' => array('title' => "Remove " . $facet))) . ")</span>";
      }
    }
    $breadcrumb[] = l(t('Home'), NULL);
    if (!empty($breadcrumb))
      $breadcrumb = array_reverse($breadcrumb);
    drupal_set_breadcrumb($breadcrumb);
    $this->solrFilters = $islandora_fq;

    $this->internalSolrQuery = $this->solrQuery;

    // replace the various null queries with a sensible catch-all.
    if(in_array($this->internalSolrQuery,$this->different_kinds_of_nothing)) {
      $this->internalSolrQuery = variable_get('islandora_solr_search_base_query', SOLR_BASE_QUERY);
      // we must also undo dismax if it's been set
      $this->solrDefType = null;
      $this->solrParams['defType'] = null;
    } else {
      // append the catch-all query to any queries which lack it?
      // No, not under normal circumstances, and it would break dismax.
    }
    // restrict results based on specified namespace
    $namespace = trim(variable_get('solr_namespace_restriction', NULL));
    if ($namespace) {
      $this->solrParams['fq'][] = "PID:$namespace\:*";
    }


    // if no qf fields are specified in rthe requestHandler a default list is supplied here for dismax searches
    if (!variable_get('dismax_allowed', 0) && $dismax == "dismax") {
      $this->solrParams['qf'] = 'dc.title^5 dc.subject^2 dc.description^2 dc.creator^2 dc.contributor^1 dc.type';
    }

    // add display to $this object
    $this->display = $display;

    // At this point let's invoke a hook for third-party modules to mess about
    // with the query parameters if they need to.  Third party modules may alter
    // this object's query parameters if they wish.
    module_invoke_all('islandora_solr_search_query_processor', $this); //The hook implementation needs to specify that it takes a reference, not be passed one
    $processor->solrParams['fq'][] = $fq;
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

    // //////////////////////////////////////////////////////////////////////////// //
    // Right here the function should be split.  One function to execute the query, //
    // another to set up pager variables and generate display.                      //
    // //////////////////////////////////////////////////////////////////////////// //

    $url = variable_get('islandora_solr_search_block_url', 'http://localhost:8080/solr');
    $pathParts = parse_url($url);

    $solr = new Apache_Solr_Service($pathParts['host'], $pathParts['port'], $pathParts['path'] . '/');
    $solr->setCreateDocuments(0);

    // This is where the query gets executed and output starts being created.
    try {
      $results = $solr->search($this->internalSolrQuery, $this->solrStart, $this->solrLimit, $this->solrParams);
    } catch (Exception $e) {
      drupal_set_message(check_plain(t('error searching')) . ' ' . $e->getMessage());
    }
    $this->solrResult = $results;
    if (empty($results)) {
      drupal_set_message(t('Error searching solr index. Is the solr search block configured properly?'), 'error');
      return;
    }

    unset($results);
    unset($solr);

    return;
  }

  /**
   * Function delete_filter
   *
   * Deletes one filter from a multi-filter string
   *
   * @param <string> $facets
   * @param <string> $facet
   */
  function delete_filter($filters, $filter) {
    $filters = str_replace($filter, "", $filters);
    $filters = str_replace("~~", "~", $filters);
    $filters = trim($filters, "~");
    return $filters;
  }

}

