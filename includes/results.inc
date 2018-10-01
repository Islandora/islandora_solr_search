<?php

/**
 * @file
 * Contains methods to create rendered Solr displays from raw Solr results.
 * Depends on Apache_Solr_Php client.
 */

/**
 * Islandora Solr Results
 */
class IslandoraSolrResults {

  public $facetFieldArray = array();
  public $searchFieldArray = array();
  public $resultFieldArray = array();
  public $allSubsArray = array();
  public $islandoraSolrQueryProcessor;
  public $rangeFacets = array();
  public $dateFormatFacets = array();

  private $backendProcessor;
  /**
   * Constructor.
   */
  public function __construct() {
    $backend_processor = islandora_solr_get_current_backend();
    $class = islandora_solr_get_namespaced_class($backend_processor, 'resultsClass');
    eval("\$this->backendProcessor = new {$class}();");
  }

  /**
   * Output the main body of the search results.
   *
   * @see islandora_solr()
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Returns themed Solr results page, including wrapper and rendered search
   *   results.
   */
  public function displayResults($islandora_solr_query) {
    return $this->backendProcessor->displayResults($islandora_solr_query);
  }

  /**
   * Renders the secondary display profile list.
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Themed list of secondary displays
   *
   * @see IslandoraSolrResults::displayResults()
   */
  public function addSecondaries($islandora_solr_query) {
    return $this->backendProcessor->addSecondaries($islandora_solr_query);

  }

  /**
   * Renders the primary or secondary display profile.
   *
   * Renders the primary or secondary display profile based on the raw Solr
   * results. This is the method most Islandora Solr display plugins would
   * override.
   *
   * @param array $solr_results
   *   The raw Solr results from
   *   IslandoraSolrQueryProcessor::$islandoraSolrResult.
   *
   * @return string
   *   Rendered Solr results
   *
   * @see IslandoraSolrResults::displayResults()
   */
  public function printResults($solr_results) {
    return $this->backendProcessor->printResults($solr_results);
  }


  /**
   * Displays elements of the current solr query.
   *
   * Displays current query and current filters. Includes a link to exclude the
   * query/filter.
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Rendered lists of the currently active query and/or filters.
   */
  public function currentQuery($islandora_solr_query) {
    return $this->backendProcessor->currentQuery($islandora_solr_query);
  }

  /**
   * Sets the Drupal breadcrumbs.
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object, which includes the current query
   *   settings and the raw Solr results.
   */
  public function setBreadcrumbs($islandora_solr_query) {
    $this->backendProcessor->setBreadcrumbs($islandora_solr_query);
  }

  /**
   * Gets the Drupal breadcrumbs.
   *
   * Gets the Drupal breadcrumbs based on the current query and filters.
   * Provides links to exclude the query or filters.
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return array
   *   An array of breadcrumbs.
   */
  public function getBreadcrumbs($islandora_solr_query) {
    return $this->backendProcessor->getBreadcrumbs($islandora_solr_query);
  }

  /**
   * Formats the passed in filter into a human readable form.
   *
   * @param string $filter
   *   The passed in filter.
   * @param object $islandora_solr_query
   *   The current Solr Query
   *
   * @return string
   *   The formatted filter string for breadcrumbs and active query.
   */
  public function formatFilter($filter, $islandora_solr_query) {
    return $this->backendProcessor->formatFilter($filter, $islandora_solr_query);
  }

  /**
   * Displays facets based on a query response.
   *
   * Includes links to include or exclude a facet field in a search.
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Rendered lists of facets including links to include or exclude a facet
   *   field.
   *
   * @see islandora_solr_islandora_solr_query_blocks()
   * @see islandora_solr_block_view()
   */
  public function displayFacets($islandora_solr_query) {
    return $this->backendProcessor->displayFacets($islandora_solr_query);
  }

  /**
   * Create a fieldset for debugging purposes.
   *
   * *** Maintained for legacy backend ***
   *
   * Creates a fieldset containing raw Solr results of the current page for
   * debugging purposes.
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $islandora_solr_results
   *   The processed Solr results from
   *   IslandoraSolrQueryProcessor::islandoraSolrResult
   *
   * @return string
   *   Rendered fieldset containing raw Solr results data.
   */
  public function printDebugOutput($islandora_solr_results) {
    if (method_exists($this->backendProcessor, 'printDebugOutput')) {
      return $this->backendProcessor->printDebugOutput($islandora_solr_results);
    }
    return "";
  }

}

/**
 * Islandora Solr Facets
 */
class IslandoraSolrFacets {
  public static $islandoraSolrQuery;

  // XXX: Need to fix the property/member variable names...  Could have an
  // effect on other code, though, due to public visibility.
  // @codingStandardsIgnoreStart
  // Normal facet results.
  public static $facet_fields;
  // Date range facet results (Solr 1.4).
  public static $facet_dates;
  // Date or integer range facet results (Solr 3.1).
  public static $facet_ranges;

  public static $facet_fields_settings;
  public static $facet_fields_settings_simple;
  // Rename?
  public static $range_facets;
  public static $minimum_count = 1;
  public static $soft_limit;
  public static $exclude_range_values;
  public static $needed_solr_call;
  public static $range_slider_key = 0;
  public static $date_filter_key = 0;

  public $facet_field;
  public $settings;
  public $facet_type;
  public $results;
  public $title = NULL;
  public $content = NULL;

  private $backendProcessor;
  private $backendClass;

  // @codingStandardsIgnoreEnd

  /**
   * Constructor method.
   *
   * Stores the facet field name, settings and title in a parameter.
   *
   * @param string $facet_field
   *   The name of the solr field to build a facet for.
   */
  public function __construct($facet_field) {
    $backend_processor = islandora_solr_get_current_backend();
    $class = islandora_solr_get_namespaced_class($backend_processor, 'facetClass');
    $this->backendClass = $class;
    eval("\$this->backendProcessor = new {$class}();");
    $this->facet_field = $facet_field;
    $this->settings = self::$facet_fields_settings[$facet_field];
    $this->title = self::$facet_fields_settings_simple[$facet_field];
  }

  /**
   * @see \Drupal\islandora_solr\SolrBackend\IslandoraSolrFacetInterface::getFacet()
   */
  public function getFacet() {
    return $this->backendProcessor->getFacet();
  }

}
