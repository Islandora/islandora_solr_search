<?php

/**
 * @file
 * Theme functions for the Islandora Solr Search Config module.
 */

/**
 * Prepares variables for islandora_solr_config templates.
 *
 * Default template: theme/islandora-solr-grid.tpl.php.
 */
function islandora_solr_config_preprocess_islandora_solr_grid(&$variables) {
  $results = $variables['results'];
  foreach ($results as $key => $result) {
    // Thumbnail.
    $path = url($result['thumbnail_url'], array('query' => $result['thumbnail_url_params']));
    $image = theme('image', array('path' => $path));

    $options = array('html' => TRUE);
    if (isset($result['object_label'])) {
      $options['attributes']['title'] = $result['object_label'];
    }
    if (isset($result['object_url_params'])) {
      $options['query'] = $result['object_url_params'];
    }
    if (isset($result['object_url_fragment'])) {
      $options['fragment'] = $result['object_url_fragment'];
    }
    // Thumbnail link.
    $variables['results'][$key]['thumbnail'] = l($image, $result['object_url'], $options);
  }
}
