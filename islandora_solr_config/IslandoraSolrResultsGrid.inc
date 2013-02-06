<?php

/**
 * @file
 * Class override for grid display
 */

/**
 * Extension of IslandoraSolrResults to create an alternative display type.
 */
class IslandoraSolrResultsGrid extends IslandoraSolrResults {

  /**
   * Renders the Solr results as a responsive grid view. Markup and styling is
   * based on the Islandora collection grid view. Some styling is inherited
   * from it.
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $results
   *   The processed Solr results from
   *   IslandoraSolrQueryProcessor::islandroaSolrResult
   *
   * @return string
   *   Rendered Solr results
   */
  function printResults($results) {
    // add islandora solr theme css
    drupal_add_css(drupal_get_path('module', 'islandora_solr_config') . '/css/islandora_solr_config.theme.css');

    // set variables
    $results_array = array();
    $elements = array();

    // total
    $elements['solr_total'] = $results['response']['numFound'];

    // start
    $elements['solr_start'] = $results['response']['start'];

    // get prepared search results
    $object_results = $this->prep_results($results);

    // loop over results array
    foreach ($object_results as $object_index => $object_result) {
      $rows = array();
      $row = 0; // keeping $row in there for now until the snippet issue is cleared out
      $snippet = NULL; // @TODO: how is this suposed to work if the $snippet always is set to NULL ?

      // loop over individual results to process fields
      foreach ($object_result['solr_doc'] as $field => $value) {

        // check for field label substitutions
        $translated_field_name = isset($this->allSubsArray[$field]) ? $this->allSubsArray[$field] : $field;

        // add label
        $rows[$field]['label'] = $translated_field_name;

        // add class: render solr field name to string that can be used as class name
        $rows[$field]['class'] = drupal_strtolower(preg_replace('/[^A-Za-z0-9]/', '-', $field));

        // split value if the result value is an array
        if (is_array($value)) {
          $value = implode(", ", $value);
        }
        // add value
        $rows[$field]['value'] = $value;

        $row++; // keeping $row in there for now until the snippet issue is cleared out
      }
      // @TODO: how is this suposed to work if the $snippet always is set to NULL ?
      if ($snippet) {
        $rows[$row][] = array(
          'data' => 'Full Text',
          'header' => TRUE,
        );
        $rows[$row][] = $snippet[0];
      }
      // Append array to object results array.
      $object_results[$object_index]['solr_doc'] = $rows;
    }

    // Return themed search results.
    return theme('islandora_solr_grid', array('results' => $object_results, 'elements' => $elements));
  }

}
