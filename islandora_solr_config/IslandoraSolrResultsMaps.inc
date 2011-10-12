<?php

/*
 * contains methods to search solr and display results.  depends on Apache_Solr_Php client.
 */

module_load_include('inc', 'islandora_solr_search', 'IslandoraSolrResults');

/**
 * Extention of IslandoraSolrResults for templating purposes.
 * This overrides the displayResults function to provide an alternate display type.
 */

class IslandoraSolrResultsMaps extends IslandoraSolrResults {

  static $facetSeparator = '~';//used to separate facets in url
  static $slashReplacement = '~slsh~';// a pattern used to replace / in url's the slash breaks drupal clean url's
  public static $SEARCH_CLASS_ADVANCED_SEARCH_NUMBER_FIELDS = 5;
  function IslandoraSolrResultsMaps() {
    parent::__construct();
  }

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
  function printResults($results) {
    $output.= '<ol start="' . ($recordStart + 1) . '">';
    global $base_url;
    if (empty($results)) {
      return "no results";
    }

    foreach ($results->response->docs as $doc) {
      $output .= '<li><table>';
      $rowspan = 0;
      $pidValue = NULL;
      $output .= '<tr>';
      $output .= '<td ><img src="' . $base_url . '/fedora/repository/' . htmlspecialchars($doc->PID, ENT_QUOTES, 'utf-8') . '/TN"/></td>';
      $output .= '<td><table>';
      foreach ($doc as $field => $value) {
        $rowspan++;
        $output .= '<tr><th>' . $field . '</th>';

        if (is_array($value)) {
          $value = implode(", ", $value);
        }
        if ($field == 'PID') {
          $pidValue = $value;
          $output.='<td><a href="' . $base_url . '/fedora/repository/' . htmlspecialchars($value, ENT_QUOTES, 'utf-8') . '">' . $value . '</td></tr>';
        } else {
          $output .= '<td>' . $value . '</td></tr>';
        }
      }

      $output .='</table></td></tr></table></li>';
    }
    $output .= '</ol>';
    return $output;
  }


}

