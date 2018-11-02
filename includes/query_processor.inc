<?php

/**
 * @file
 * Contains methods to build and execute a solr query. Depends on
 * Apache_Solr_Php client.
 */

use Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface;

/**
 * Islandora Solr Query Processor.
 *
 * Used to store Solr query parameters and to connect to Solr to execute the
 * query. Populates the islandoraSolrResult property with the processed Solr
 * query results.
 */
class IslandoraSolrQueryProcessor implements IslandoraSolrQueryInterface {

  public $differentKindsOfNothing = array(
    ' ',
    '%20',
    '%252F',
    '%2F',
    '%252F-',
    '',
  );

  private $backendProcessor;

  // Member variables now expected from the lower implementation.
  private $proxied = array(
    // Solr results tailored for Islandora's use.
    'islandoraSolrResult',
    'solrQuery',
    // Query alternative set if solrQuery is empty.
    'internalSolrQuery',
    'solrStart',
    'solrLimit',
    'solrDefType',
    // All other Solr parameters.
    'solrParams',
    // The current display.
    // (for modules wanting to alter the query of a display).
    'display',
    // Parameters from URL.
    'internalSolrParams',
  );

  /**
   * Handle deprectation of old class member gracefully.
   *
   * Also handle proxied gets. Ampersand is needed for direct array referencing.
   */
  public function &__get($name) {
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

    if (in_array($name, $this->proxied)) {
      return $this->backendProcessor->$name;
    }
  }

  /**
   * Proxying access to member results.
   *
   * @param string $name
   *   The name of the variable to check.
   *
   * @return bool
   *   True if set.
   */
  public function __isset($name) {
    if (in_array($name, $this->proxied)) {
      return isset($this->backendProcessor->$name);
    }
  }

  /**
   * Magic setter.
   *
   * @param string $name
   *   The member variable.
   * @param mixed $value
   *   The new value.
   */
  public function __set($name, $value) {
    if (in_array($name, $this->proxied)) {
      $this->backendProcessor->$name = $value;
    }
  }

  /**
   * IslandoraSolrQueryProcessor constructor.
   */
  public function __construct() {
    $backend_processor = islandora_solr_get_current_backend();
    $class = islandora_solr_get_namespaced_class($backend_processor, 'queryClass');
    $this->backendProcessor = new $class();
  }

  /**
   * {@inheritdoc}
   */
  public function buildAndExecuteQuery($query, $params = NULL, $alter_results = TRUE) {
    $this->backendProcessor->buildAndExecuteQuery($query, $params, $alter_results);
  }

  /**
   * {@inheritdoc}
   */
  public function buildQuery($query, $params = array()) {
    $this->backendProcessor->buildQuery($query, $params);
  }

  /**
   * {@inheritdoc}
   */
  public function resetResults() {
    $this->backendProcessor->resetResults();
  }

  /**
   * {@inheritdoc}
   */
  public function executeQuery($alter_results = TRUE, $use_post = FALSE) {
    $this->backendProcessor->executeQuery($alter_results, $use_post);
  }

  /**
   * {@inheritdoc}
   */
  public function prepareSolrDoc($object_results) {
    return $this->backendProcessor->prepareSolrDoc($object_results);
  }

  /**
   * {@inheritdoc}
   */
  public function ping($solr_url) {
    return $this->backendProcessor->ping($solr_url);
  }
}
