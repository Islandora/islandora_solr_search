<?php

/**
 * @file
 * Contains methods to search solr and display results.  depends on Apache_Solr_Php client.
 */

module_load_include('inc', 'islandora_solr_search', 'IslandoraSolrResults');

/**
 * Extention of IslandoraSolrResults for templating purposes.
 * This overrides the displayResults function to provide an alternate display type.
 */

class IslandoraSolrResultsRSS extends IslandoraSolrResults {

  /**
   * Outputs results basically in the normal way, but with thumbnails pulled
   * from the Fedora repository.
   *
   * @param $results
   *   A solr resultset object.
   *
   * @return
   *   html output for the resultset. Note: we currently create this
   *   output manually, should refactor to use drupal forms api.
   */
  function printRSS($solrQueryProcessor, $title = "Search Results") {

    global $base_url;

    // get raw results
    $solr_result = $solrQueryProcessor->solrResult;
    $raw_response = $solr_result->getRawResponse();
    // object to json
    $results_json = json_encode($raw_response);
    // json  to array
    $results_array = json_decode($results_json, TRUE);
    $results_array = json_decode($results_array, TRUE); // not sure why I need to decode it twice to make it work properly.

    // all results
    $docs = $results_array['response']['docs'];

    // loop over results
    $items = NULL;
    foreach ($docs as $doc) {
      // turn arrays into strings
      foreach ($doc as $key => $value) {
        if (is_array($value)) {
          // turn array into comma separated string and trim
          $doc[$key] = trim(implode(', ', $value));
        }
        else {
          // give it a trim
          $doc[$key] = trim($value);
        }
      }
      // get the variables for the <item> element
      $item = $this->rssItem($doc);

      //hook alter to change the rssItem before formatting.
      drupal_alter('islandora_solr_search_rss_item', $item, $doc);

      // render rss item and add to items string
      $items .= format_rss_item($item['title'], $item['link'], $item['description'], $item['items']);
    }

    // query search terms:
    $query = $solrQueryProcessor->solrQuery;

    // get the variables for the <channel> element
    $channel = $this->rssChannel($query);

    // give the results clean variable names.
    $title = $channel['title'];
    $url = $channel['url'];
    $description = $channel['description'];
    $langcode = $channel['langcode'];
    $args = $channel['args'];

    $output = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    $output .= "<rss version=\"2.0\">\n";
    $output .= format_rss_channel($title, $url, $description, $items, $langcode, $args);
    $output .= "</rss>\n";

    print $output;
    exit;
  }


  /**
   * Function for setting the values of the <item> elements for the RSS secondary display.
   *
   * @tutorial http://feed2.w3.org/docs/rss2.html#hrelementsOfLtitemgt
   * @return variable that holds all values to be rendered into <item> elements
   */

  function rssItem($doc) {
    // set variables
    global $base_url;

    $rss_item = variable_get('islandora_solr_config_rss_item', array());

    // object url
    $object_url = $base_url . '/fedora/repository/' . htmlspecialchars($doc['PID'], ENT_QUOTES, 'utf-8');

    // enclosure file url (thumbnail by default)
    $dsid = $rss_item['enclosure_dsid'] ? $rss_item['enclosure_dsid'] : 'TN';
    $enclosure_url = $object_url . '/' . $dsid;

    // set the variables to be rendered as elements in the <item> element
    $result = array();
    $result['title'] = $rss_item['title'] ? $doc[$rss_item['title']] : '';
    $result['link'] = $object_url;
    $result['description'] = $rss_item['description'] ? $doc[$rss_item['description']] : '';
    $result['items'] = array(
      array('key' => 'author', 'value' => $rss_item['author'] ? $doc[$rss_item['author']] : ''),
      array('key' => 'guid', 'value' => $doc['PID'], 'attributes' => array('isPermaLink' => 'false')),
      array('key' => 'pubDate', 'value' => $rss_item['pubDate'] ? $doc[$rss_item['pubDate']] : ''),
      array('key' => 'category', 'value' => $rss_item['category'] ? $doc[$rss_item['category']] : ''),
      array('key' => 'enclosure', 'value' => '', 'attributes' => array('url' => $enclosure_url, 'length' => '', 'type' => '')),
      array('key' => 'comments', 'value' => ''),
      array('key' => 'source', 'value' => ''),
    );
    return $result;
  }

  /**
   * Function for setting the values of the <channel> elements for the RSS secondary display.
   *
   * @tutorial http://feed2.w3.org/docs/rss2.html#requiredChannelElements
   * @return variable that holds all values to be rendered into <channel> elements
   */

  function rssChannel($query) {
    // set variables
    global $base_url;
    $rss_channel = variable_get('islandora_solr_config_rss_channel', array());

    // set the variables to be rendered as elements in the <channel> element
    $result = array();
    $result['title'] = t('@site_name aggregator', array('@site_name' => variable_get('site_name', 'Drupal')));
    $result['url'] = $base_url;
    $result['description'] = t('Aggregated search results of: @query', array('@query' => $query));
    $result['langcode'] = NULL;
    $result['args'] = array(
        array('key' => 'copyright', 'value' => $rss_channel['copyright'] ? $doc[$rss_channel['copyright']] : ''),
        array('key' => 'managingEditor', 'value' => $rss_channel['managingEditor'] ? $doc[$rss_channel['managingEditor']] : ''),
        array('key' => 'webMaster', 'value' => $rss_channel['webMaster'] ? $doc[$rss_channel['webMaster']] : ''),
    );
    return $result;
  }
}
