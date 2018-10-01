<?php
/**
 * @file
 * Solr Results interface.
 */

namespace Drupal\islandora_solr\SolrBackend;

/**
 * Interface IslandoraSolrResultsInterface
 * @package Drupal\islandora_solr\SolrBackend
 */
interface IslandoraSolrResultsInterface {

  /**
   * Output the main body of the search results.
   *
   * @see islandora_solr()
   *
   * @param \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface $islandora_solr_query
   *   The IslandoraSolrQueryInterface object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Returns themed Solr results page, including wrapper and rendered search
   *   results.
   */
  public function displayResults($islandora_solr_query);

  /**
   * Renders the secondary display profile list.
   *
   * @param \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface $islandora_solr_query
   *   The IslandoraSolrQueryInterface object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Themed list of secondary displays
   *
   * @see IslandoraSolrResults::displayResults()
   */
  public function addSecondaries($islandora_solr_query);

  /**
   * Renders the primary or secondary display profile.
   *
   * Renders the primary or secondary display profile based on the raw Solr
   * results. This is the method most Islandora Solr display plugins would
   * override.
   *
   * @param array $solr_results
   *   The raw Solr results from
   *   IslandoraSolrQueryInterface::$islandoraSolrResult.
   *
   * @return string
   *   Rendered Solr results
   *
   * @see IslandoraSolrResults::displayResults()
   */
  public function printResults($solr_results);

  /**
   * Displays elements of the current solr query.
   *
   * Displays current query and current filters. Includes a link to exclude the
   * query/filter.
   *
   * @param \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface $islandora_solr_query
   *   The IslandoraSolrQueryInterface object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Rendered lists of the currently active query and/or filters.
   */
  public function currentQuery($islandora_solr_query);

  /**
   * Sets the Drupal breadcrumbs.
   *
   * @param \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface $islandora_solr_query
   *   The IslandoraSolrQueryInterface object, which includes the current query
   *   settings and the raw Solr results.
   */
  public function setBreadcrumbs($islandora_solr_query);

  /**
   * Gets the Drupal breadcrumbs.
   *
   * Gets the Drupal breadcrumbs based on the current query and filters.
   * Provides links to exclude the query or filters.
   *
   * @param \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface $islandora_solr_query
   *   The IslandoraSolrQueryInterface object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return array
   *   An array of breadcrumbs.
   */
  public function getBreadcrumbs($islandora_solr_query);

  /**
   * Formats the passed in filter into a human readable form.
   *
   * @param string $filter
   *   The passed in filter.
   * @param \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface $islandora_solr_query
   *   The current Solr Query
   *
   * @return string
   *   The formatted filter string for breadcrumbs and active query.
   */
  public function formatFilter($filter, $islandora_solr_query);

  /**
   * Displays facets based on a query response.
   *
   * Includes links to include or exclude a facet field in a search.
   *
   * @param \Drupal\islandora_solr\SolrBackend\IslandoraSolrQueryInterface $islandora_solr_query
   *   The IslandoraSolrQueryInterface object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Rendered lists of facets including links to include or exclude a facet
   *   field.
   *
   * @see islandora_solr_islandora_solr_query_blocks()
   * @see islandora_solr_block_view()
   */
  public function displayFacets($islandora_solr_query);

}