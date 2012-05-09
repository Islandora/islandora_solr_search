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
  $variables['base_url'] = $base_url;
  
  // elements
  $elements = $variables['elements'];
  
  // make all variables in 'elements' available as variables in the template file
  foreach ($variables['elements'] as $key => $value) {
    $variables[$key] = $value;
  }
  
}
