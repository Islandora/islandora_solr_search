<?php

/**
 * @file
 *   Contains methods to search solr and display results. Depends on
 *   Apache_Solr_Php client.
 */

/**
 * Extension of IslandoraSolrResults to create an alternative display type.
 */
class IslandoraSolrResultsTable extends IslandoraSolrResults {

  /**
   * Constructor: calls parent.
   *
   * @return NULL
   *   Nothing to return.
   */
  function __construct() {
    parent::__construct();
    return;
  }

  /**
   * Renders the Solr results as a table.
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $results
   *   The raw Solr results from
   *   IslandoraSolrQueryProcessor::islandoraSolrResult
   *
   * @return string
   *   Rendered Solr results
   */
  function printResults($results) {

    // add Islandora solr theme css
    drupal_add_css(drupal_get_path('module', 'islandora_solr_config') . '/css/islandora_solr_config.theme.css');

    $output = '';

    $record_start = (int) $results['response']['start'];
    if (empty($results)) {
      return t('no results');
    }
    $count = 0;
    $fields = array();
    $values = array();
    $header = array();
    $limit_results = variable_get('islandora_solr_limit_result_fields', 0);

    foreach ($results['response']['objects'] as $object_result) {
      $doc = $object_result['solr_doc'];
      unset($pid);
      foreach ($doc as $field => $value) {

        if ($limit_results && empty($this->resultFieldArray[$field])) {
          continue;
        }

        // catches the object PID.
        if ($field == 'PID') {
          $pid = $value;
          // we need this in the resultset, but would prefer to hide it from users.
        }

        // Sometimes we get a single value, sometimes an array.
        // For simplicity, we normalize to an array.
        if (!is_array($value)) {
          $value = array($value);
        }

        // We're going to put the object link onto the dublin-core title.
        if ($field == 'dc.title' || $field == 'dc:title') {
          foreach ($value as $key => $val) {
            if (isset($doc->PID)) {
              $value[$key] = l($val, 'islandora/object/' . $doc->PID . '/-/' . $val);
            }
          }
        }
        $value = implode(", ", $value);

        // Add this field to the master list of fields if it's not there.
        if (!in_array($field, $fields)) {
          $fields[] = $field;
        }

        $values[$count][$field] = $value;
      }
      $count++;
    }

    foreach ($fields as $field) {
      // Get human-friendly field names;
      $translated_field_name = isset($this->allSubsArray[$field]) ? $this->allSubsArray[$field] : $field;
      $field_array = array(
        'data' => $translated_field_name,
        'field' => NULL,
        'sort' => NULL,
        'title' => $field,
      );
      $header[] = $field_array;
    }
    array_unshift($header, '#');

    $rows = array();
    // Loop over each row.
    foreach ($values as $num => $val_array) {
      $row = array();
      unset($rowclass);
      $result_num = $record_start + $num + 1;
      $row['data'][] = array(
        'data' => (isset($val_array['PID']) ? l($result_num, 'islandora/object/' . $val_array['PID']) : $result_num),
        'header' => TRUE,
      );
      $field_count = count($fields);
      // Loop over each cell within a row.
      foreach ($fields as $fieldnum => $field) {
        $data = isset($val_array[$field]) ? $val_array[$field] : ' ' ;
        $cell_data = array('data' => $data);

        $row['data'][] = $cell_data;

      }
      $rows[] = $row;
    }
    $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('class' => array('islandora-solr-results-table'))));

    return $output;
  }
}
