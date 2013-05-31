<?php

/**
 * @file
 * Hooks provided by Islandora Solr.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Hook to collect data to set up a primary display profile.
 *
 * @return array
 *   Returns an associative array mapping unique machine names to associative
 *   arrays with all the data required to build a primary display profile, such
 *   as:
 *   - "name": A translated string to display.
 *   - "module": The module where the code for the given primary display is
 *     located. Optional if using autoclassloading.
 *   - "file": A file under the given module's path to load before trying to
 *     instantiate the display. Optional if using autoclassloading.
 *   - "class": The name of the class to try to instantiate. Should be a
 *     subclass of IslandoraSolrResults.
 *   - "function": The name of a method/function to call on the instantiated
 *     object.
 *   - "description": A translated string containing a description about the
 *     of the given display.
 *   - "configuration": A which can be used to configure the given display.
 */
function hook_islandora_solr_primary_display() {
  return array(
    'machine_name' => array(
      'name' => t('Human-readable name'),
      'module' => 'module_name',
      'file' => 'FileName.inc',
      'class' => 'ClassName',
      'function' => 'function_name',
      'description' => t('A description of the display profile'),
      'configuration' => 'path/to/configuration/page',
    ),
  );
}

/**
 * Hook to collect data to set up a secondary display profile.
 *
 * @return array
 *   Returns an associative array mapping unique machine names to associative
 *   arrays with all the data required to build a primary display profile, such
 *   as:
 *   - "name": A translated string to display.
 *   - "module": The module where the code for the given display is located.
 *   - "file": A file under the given module's path to load before trying to
 *     instantiate the display.
 *   - "class": The name of the class to try to instantiate. Should be a
 *     subclass of IslandoraSolrResults.
 *   - "function": The name of a method/function to call on the instantiated
 *     object.
 *   - "description": A translated string containing a description about the
 *     of the given display.
 *   - "logo": An opening image tag to use a a logo.
 */
function hook_islandora_solr_secondary_display() {
  return array(
    'machine_name' => array(
      'name' => t('Human-readable name'),
      'module' => 'module_name',
      'file' => 'FileName.inc',
      'class' => 'ClassName',
      'function' => 'function_name',
      'description' => t('A description of the display profile'),
      'logo' => '<img src="path/to/icon.png">',
    ),
  );
}

/**
 * Hook which passes the IslandoraSolrQueryProcessor object for modification.
 *
 * This hook is called at the end of IslandoraSolrQueryProcessor::buildQuery()
 * and it's purpose is to make 'last minute' changes. This will not be called
 * when creating a query processor object programatically without manually
 * including IslandoraSolrQueryProcessor::buildQuery().
 *
 * @param IslandoraSolrQueryProcessor $islandora_solr_query
 *   The IslandoraSolrQueryProcessor object which includes the current query
 *   settings but at the end of IslandoraSolrQuery::buildQuery().
 *
 * @see IslandoraSolrQueryProcessor::buildQuery()
 */
function hook_islandora_solr_query($islandora_solr_query) {
  // example: on example_display, always sort descending on fgs.createdDate
  if ($islandora_solr_query->display == 'example_display') {
    $islandora_solr_query->solrParams['sort'] = 'fgs.createdDate desc';
  }
}

/**
 * Hook to collect data to populate block Islandora Solr blocks.
 *
 * Note: it is valid to specify *either* a class and method *or* a form. The
 * latter implies no class needs to be instantiated.
 *
 * @return array
 *   Returns an array with all the data required to build blocks. These include
 *   references to specific modules/methods/classes/functions to call in
 *   hook_block_view().
 */
function hook_islandora_solr_query_blocks() {
  return array(
    'machine_name' => array(
      'name' => t('Human-Readable Name'),
      'module' => 'module_name',
      'file' => 'FileName.inc',
      'class' => 'ClassName',
      'function' => 'method_name',
      'form' => 'form_function_name',
    ),
  );
}

/**
 * @} End of "addtogroup hooks".
 */


/**
 * Implements hook_CMODEL_PID_islandora_solr_object_result_alter().
 */
function hook_islandora_pageCModel_islandora_solr_object_result_alter(&$search_results, $query_processor) {
  $search_results['object_url_params']['terms'] = $query_processor->solrQuery;
}

/**
 * Implements hook_islandora_solr_results_alter().
 */
function hook_islandora_solr_results_alter(&$search_results, $query_processor) {
  $search_results['object_url_params']['terms'] = $query_processor->solrQuery;
}

/**
 * Implements hook_islandora_solr_object_result_alter().
 */
function hook_islandora_solr_object_result_alter(&$search_results, $query_processor) {
  $search_results[0]['object_url_params']['terms'] = $query_processor->solrQuery;
}
