<?php
/**
 * @file
 * IslandoraSolrQueryInterface
 */

namespace Drupal\islandora_solr\SolrBackend;

/**
 * Interface for parsing and displaying the results from Solr.
 * @package Drupal\islandora_solr\SolrBackend
 */
interface IslandoraSolrQueryInterface {

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
  public function buildAndExecuteQuery($query, $params = NULL, $alter_results = TRUE);

  /**
   * Builds Solr query.
   *
   * Build the query and performs checks based on URL parameters and
   * defaults set in the Islandora Solr admin form. Populates the properties to
   * be used for the query execution. Includes a module_invoke_all to make
   * changes to the query.
   *
   * @see IslandoraSolrQueryProcessor::buildAndExecuteQuery()
   *
   * @param string $query
   *   The query string provided in the URL.
   * @param array $params
   *   All URL parameters from the Solr results page.
   */
  public function buildQuery($query, $params = array());

  /**
   * Reset results.
   */
  public function resetResults();

  /**
   * Connects to Solr and executes the query.
   *
   * Populates islandoraSolrResults property with the raw Solr results.
   *
   * @param bool $alter_results
   *   Whether or not to send out hooks to alter the islandora_solr_results.
   */
  public function executeQuery($alter_results = TRUE, $use_post = FALSE);

  /**
   * Filter all Solr docs.
   *
   * Iterates of the Solr doc of every result object and applies filters
   * sort orders.
   *
   * @param array $object_results
   *   An array containing the prepared object results.
   *
   * @return array
   *   The object results array with updated solr doc values.
   */
  public function prepareSolrDoc($object_results);

  /**
   * Pings a Solr instance for availability.
   *
   * @param string $solr_url
   *   A URL that points to Solr.
   *
   * @return int|bool
   *   Returns ping time in milliseconds on success or boolean FALSE if Solr
   *   could not be reached.
   */
  public function ping($solr_url);

}
