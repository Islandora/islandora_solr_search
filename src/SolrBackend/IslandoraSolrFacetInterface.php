<?php
/**
 * @file
 * Solr Facet interface.
 */

namespace Drupal\islandora_solr\SolrBackend;

/**
 * Interface IslandoraSolrFacetInterface
 * @package Drupal\islandora_solr\SolrBackend
 */
interface IslandoraSolrFacetInterface {

  /**
   * Constructor method.
   *
   * Stores the facet field name, settings and title in a parameter.
   *
   * @param string $facet_field
   *   The name of the solr field to build a facet for.
   */
  public function __construct($facet_field);

  /**
   * Prepare and render facet.
   *
   * Method called after a facet object is created. This will prepare the
   * results based on the type and user settings for this facet. It also does a
   * call to render the prepared data. This method also returns the rendered
   * endresult.
   *
   * @return string
   *   Returns the title and rendered facet.
   */
  public function getFacet();

}
