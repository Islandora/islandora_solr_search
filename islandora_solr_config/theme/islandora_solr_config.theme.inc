<?php

/**
 * @file
 * Theme functions for Islandora Solr Config
 */

/**
 * hook_preprocess function for islandora_solr_grid
 *
 * @global type $base_url
 * @global type $base_path
 * @param type $variables
 *   An associative array containing:
 *   - results: array of returned Solr documents
 *   - elements: (optional) additional elements to include in the template file
 *
 * @see islandora_solr_config_theme()
 * @see IslandoraSolrResultsGrid::printResults()
 */
function islandora_solr_config_preprocess_islandora_solr_grid(&$variables) {
  // base url
  global $base_url;
  // base path
  global $base_path;

  // set base url
  $variables['base_url'] = $base_url;

  // set base path
  $variables['base_path'] = $base_path;

  // get path
  $path = $base_path . drupal_get_path('module', 'islandora_solr');

  // placeholder image
  $solr_default_img = $path . '/images/defaultimg.png';
  $variables['elements']['solr_default_img'] = $solr_default_img;

  // make all variables in 'elements' available as variables in the template file
  foreach ($variables['elements'] as $key => $value) {
    $variables[$key] = $value;
  }

  foreach ($variables['results'] as $key => $value) {
    $object = islandora_object_load($value['PID']['value']);
    if ($object) {
      if (isset($object['TN'])) {
        $pid = $variables['results'][$key]['PID']['value'];
        $variables['thumbnail_path'][$key] = url('islandora/object/' . $pid . '/datastream/TN');
      }
      else {
        $variables['thumbnail_path'][$key] = $path . '/images/defaultimg.png';
      }
    }
    else {
      $variables['thumbnail_path'][$key] = $path . '/images/defaultimg.png';
    }
  }
}