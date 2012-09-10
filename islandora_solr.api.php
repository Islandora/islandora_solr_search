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
 * @return
 *   Returns an array with all the data required to build a primary display
 *   profile.
 */
function hook_islandora_solr_primary_display() {
  return array(
    'machine_name' => array(
      'name' => t('Human readable name'),
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
 * @return
 *   Returns an array with all the data required to build a secondary display
 *   profile.
 */
function hook_islandora_solr_secondary_display() {
  return array(
    'machine_name' => array(
      'name' => t('Human readable name'),
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
 * @param type $islandora_solr_query
 *   The IslandoraSolrQueryProcessor object which includes the current query
 *   settings but at the end of IslandoraSolrQuery::buildQuery().
 *
 * @see IslandoraSolrQueryProcessor::buildQuery().
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
 * @return
 *   Returns an array with all the data required to build blocks. These include
 *   references to specific modules/methods/classes/functions to call in
 *   hook_block_view().
 */
function hook_islandora_solr_query_blocks() {
  return array(
    'machine_name' => array(
      'name' => t('Human Readable Name'),
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
