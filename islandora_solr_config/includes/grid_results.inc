<?php

/**
 * @file
 * Class override for grid display
 */

/**
 * Extension of IslandoraSolrResults to create an alternative display type.
 */
class IslandoraSolrResultsGrid extends IslandoraSolrResults {

  /**
   * Renders the Solr results as a responsive grid view. Markup and styling is
   * based on the Islandora collection grid view. Some styling is inherited
   * from it.
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $results
   *   The processed Solr results from
   *   IslandoraSolrQueryProcessor::islandroaSolrResult
   *
   * @return string
   *   Rendered Solr results
   */
  function printResults($solr_results) {
    // add islandora solr theme css
    drupal_add_css(drupal_get_path('module', 'islandora_solr_config') . '/css/islandora_solr_config.theme.css');
    $object_results = $solr_results['response']['objects'];
    $elements = array();
    // total
    $elements['solr_total'] = $solr_results['response']['numFound'];
    // start
    $elements['solr_start'] = $solr_results['response']['start'];
    // Return themed search results.
    return theme('islandora_solr_grid', array('results' => $object_results, 'elements' => $elements));
  }
}
