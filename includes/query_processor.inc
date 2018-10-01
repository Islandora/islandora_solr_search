<?php

/**
 * @file
 * Contains methods to build and execute a solr query. Depends on
 * Apache_Solr_Php client.
 */

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
  // The current display (for modules wanting to alter the query of a display).
  public $display;
  // Parameters from URL.
  public $internalSolrParams;
  public $differentKindsOfNothing = array(
    ' ',
    '%20',
    '%252F',
    '%2F',
    '%252F-',
    '',
  );

  private $backendProcessor;

  /**
   * Handle deprectation of old class member gracefully.
   */
  public function __get($name) {
    $map = array(
      'different_kinds_of_nothing' => 'differentKindsOfNothing',
    );

    if (isset($map[$name])) {
      $new_name = $map[$name];
      $trace = debug_backtrace();

      $message = t('Use of variable name "@class->@old_name" on line @line of @file deprecated as of version @version. Refactor to use "@class->@name" before the next release.', array(
        '@old_name' => $name,
        '@name' => $new_name,
        '@class' => __CLASS__,
        '@version' => '7.x-1.2',
        '@line' => $trace[0]['line'],
        '@file' => $trace[0]['file'],
      ));

      trigger_error($message, E_USER_DEPRECATED);

      return $this->$new_name;
    }

    if ($name == 'islandoraSolrResult') {
      // Pass the result from the proxied query processor for backwards compatibility.
      return $this->backendProcessor->islandoraSolrResult;
    }
  }

  /**
   * Build and execute a query.
   *
   * @param string $query
   *   The query string provided in the url.
   * @param array $params
   *   All URL parameters from the solr results page.
   * @param bool $alter_results
   *   Whether or not to send out hooks to alter the islandora_solr_results.
   */
  public function __construct() {
    $backend_processor = islandora_solr_get_current_backend();
    $class = islandora_solr_get_namespaced_class($backend_processor, 'queryClass');
    eval("\$this->backendProcessor = new {$class}();");
  }

  /**
   * @see \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface::buildAndExecuteQuery()
   */
  public function buildAndExecuteQuery($query, $params = NULL, $alter_results = TRUE) {
    $this->backendProcessor->buildAndExecuteQuery($query, $params, $alter_results);
  }

  /**
   * @see \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface::buildQuery()
   */
  public function buildQuery($query, $params = array()) {
    $this->backendProcessor->buildQuery($query, $params);
  }

  /**
   * @see \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface::resetResults()
   */
  public function resetResults() {
    $this->backendProcessor->resetResults();
  }

  /**
   * @see \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface::executeQuery()
   */
  public function executeQuery($alter_results = TRUE, $use_post = FALSE) {
    $this->backendProcessor->executeQuery($alter_results, $use_post);
  }

}
