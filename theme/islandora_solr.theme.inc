<?php

/**
 * @file
 * Theme functions for the Islandora Solr Search module.
 */


/**
 * hook_preprocess function for islandora_solr_wrapper
 *
 * @global type $base_url
 * @param type $variables
 *   An associative array containing:
 *   - results: Rendered primary display
 *   - secondary_profiles: (optional) Rendered list of secondary display
 *     profiles
 *   - elements: additional elements to populate the results page
 *     - pager: (optional) Rendered pager
 *     - solr_total: (optional) Total amount of documents found.
 *     - solr_start: (optional) Solr start position
 *     - solr_end: (optional) Solr end position
 *
 * @see islandora_solr_theme()
 * @see IslandoraSolrResults::displayResults()
 */
function islandora_solr_preprocess_islandora_solr_wrapper(&$variables) {
  // base url
  global $base_url;
  $variables['base_url'] = $base_url;

  // elements
  $elements = $variables['elements'];

  // make all variables in 'elements' available as variables in the template file
  foreach ($variables['elements'] as $key => $value) {
    $variables[$key] = $value;
  }
  // results count
  $total = $elements['solr_total'];
  $end = $elements['solr_end'];
  $start = $elements['solr_start'] + ($total > 0 ? 1 : 0);

  // format numbers
  $total = number_format($total , 0, '.', ',');
  $end = number_format($end , 0, '.', ',');
  $start = number_format($start , 0, '.', ',');
  // construct string
  $islandora_solr_result_count = t('(@start - @end of @total)', array('@start' => $start, '@end' => $end, '@total' => $total));
  // add to variables
  $variables['islandora_solr_result_count'] = $islandora_solr_result_count;

}



/**
 * hook_preprocess function for islandora_solr_default
 *
 * @global type $base_url
 * @global type $base_path
 * @param type $variables
 *   An associative array containing:
 *   - results: array of returned Solr documents
 *   - elements: (optional) additional elements to include in the template file
 *
 * @see islandora_solr_theme()
 * @see IslandoraSolrResults::printResults()
 */
function islandora_solr_preprocess_islandora_solr(&$variables) {
  // base url
  global $base_url;
  // base path
  global $base_path;
  // set base url
  $variables['base_url'] = $base_url;
  // set base path
  $variables['base_path'] = $base_path;
  // pids
  $pids = $variables['pids'];
  // get path
  $path = $base_path . drupal_get_path('module', 'islandora_solr');
  // placeholder image
  $solr_default_img = $path . '/images/defaultimg.png';
  $variables['elements']['solr_default_img'] = $solr_default_img;
  // make all variables in 'elements' available as variables in the template file
  foreach ($variables['elements'] as $key => $value) {
    $variables[$key] = $value;
  }
  // thumbnails
  foreach ($variables['results'] as $key => $value) {
    $pid = $pids[$key];
    $object = islandora_object_load($pids[$key]);
    if ($object) {
      if (isset($object['TN'])) {
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
