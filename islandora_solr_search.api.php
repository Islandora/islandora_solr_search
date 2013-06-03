<?php
/**
 * @file
 * This file defines the hooks that islandora_solr_search
 * makes available.
 */

/**
 * This hook allows modules to edit an rss_item.
 *
 * Somtimes you might want to alter how an rss item is displayed.
 *
 * @param rssItem $item
 *   The rssItem object
 * @param array $doc
 *   The solr results document
 */
function hook_islandora_solr_search_rss_item_alter($item, $doc) {

  $item['title'] = $doc['PID'];
  $item['description'] = 'this is the new rss item description';
}
