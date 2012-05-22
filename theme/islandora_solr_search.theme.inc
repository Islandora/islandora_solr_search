<?php

/**
 * @file
 * Theme functions for the Islandora Solr Search module.
 */

// page + elements
// islandora-solr-search.tpl.php
// 
// solr search results
// islandora-solr-search-default.tpl.php

/**
 * hook_preprocess function for islandora_solr_search
 */
function islandora_solr_search_preprocess_islandora_solr_wrapper(&$variables) {
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
  // construct string
  $islandora_solr_result_count = t('(@start - @end of @total)', array('@start' => $start, '@end' => $end, '@total' => $total));
  // add to variables
  $variables['islandora_solr_result_count'] = $islandora_solr_result_count;

}



/**
 * hook_preprocess function for islandora_solr_search_default
 */
function islandora_solr_search_preprocess_islandora_solr_search(&$variables) {
  // base url
  global $base_url;
  // base path
  global $base_path;

  // Load external code to probe object datastreams
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  // set base url
  $variables['base_url'] = $base_url;

  // set base path
  $variables['base_path'] = $base_path;
  
  // get path
  $path = $base_path . drupal_get_path('module', 'islandora_solr_search');
  
  // placeholder image
  $solr_default_img = $path . '/images/defaultimg.png';
  $variables['elements']['solr_default_img'] = $solr_default_img;

  // make all variables in 'elements' available as variables in the template file
  foreach ($variables['elements'] as $key => $value) {
    $variables[$key] = $value;
  }

  // detect existing datastreams and create a linked thumbnail if TN exists
  foreach($variables['results'] as $key => $value) {
    $object = new Fedora_Item($value['PID']['value']);
    $ds_array = $object->get_datastreams_list_as_array();
    if (array_key_exists('TN', $ds_array) && (isset($variables['results'][$key]['PID']['value']))) {
      $pid = $variables['results'][$key]['PID']['value'];
      $solr_object_link = 'fedora/repository/' . $pid;
      $solr_thumbnail_img = '<img src="' . $base_path . $solr_object_link . '/TN"' . '/>';
      $variables['solr_thumbnail'][$key] = l($solr_thumbnail_img, $solr_object_link, array('html' => TRUE, 'attributes' => array('title' => $variables['results'][$key]['dc.title']['value'])));
    }
    else {
      $variables['solr_thumbnail'][$key] = '<img src="' . $path . '/images/defaultimg.png' . '"/>';
    }
  }
  
}
