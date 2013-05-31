<?php

/**
 * @file
 * Contains methods to search solr and display results. Depends on
 * Apache_Solr_Php client.
 */

/**
 * Extention of IslandoraSolrResults to create an alternative display type.
 */
class IslandoraSolrResultsCSV extends IslandoraSolrResults {

  /**
   * Renders the Solr results as a comma separated values file (.csv).
   *
   * Renders the Solr results as a comma separated values file (.csv). Resets
   * the html headers so it'll prompt to be downloaded.
   *
   * @see IslandoraSolrResults
   *
   * @global type $base_url
   *
   * @param object $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   */
  public function printCSV($islandora_solr_query) {
    global $base_url;
    $result_fields = $this->resultFieldArray;

    // First off, update outer limits.
    $upper_limit = $islandora_solr_query->islandoraSolrResult['response']['numFound'];
    $lower_limit = 0;

    // Typical failure point for an unbounded query seems to be around
    // 10000-15000, but we must allow for the every-growing output memory
    // structure.
    $increment = 5000;

    $row_count = 0;
    $field_counts = array();
    $values = array();

    $seperator = ",";
    $wrap = '"';
    $replace_value = '""';

    // Note: solrLimit is really stating the number of rows wanted,
    // not the row number of the upper limit document.  That is, if you
    // want results 40-60, you set solrStart=40, solrLimit=20 -- *not*
    // solrStart=40, solrLimit=60.
    $islandora_solr_query->solrStart = $lower_limit;
    $islandora_solr_query->solrLimit = $increment;

    $docfile = tmpfile();

    while ($islandora_solr_query->solrStart < $upper_limit) {

      // Perform the incremental re-query.
      $islandora_solr_query->resetResults();
      $islandora_solr_query->executeQuery();

      // Update incremental limits for the next round.
      $lower_limit += $increment;
      $islandora_solr_query->solrStart = $lower_limit;

      // If there are no results, skip ahead (i.e. exit).
      if (empty($islandora_solr_query->islandoraSolrResult)) {
        continue;
      }

      // Do the work for this iteration.
      $response_data = $islandora_solr_query->islandoraSolrResult['response'];

      // This loop is just to update the maximum value counts for each field
      // because if any field in the entire resultset has multiple values,
      // we must break the field into multiple iterated fields.
      $object_results = $response_data['objects'];
      $object_results = islandora_solr_prepare_solr_doc($object_results);
      foreach ($object_results as $object_result) {

        $doc = $object_result['solr_doc'];
        fwrite($docfile, addcslashes(serialize($doc), "\n\r\\") . "\n");
        foreach ($doc as $field => $value) {

          $field_count = is_array($value['value']) ? count($value['value']) : 1;

          if (!isset($field_counts[$field])) {
            $field_counts[$field] = $field_count;
          }
          else {
            $field_counts[$field] = max($field_counts[$field], $field_count);
          }
        }
        $row_count++;
      }

      // Let's try writing a serialized version of the each doc to a file and
      // removing it from memory, then digging through said file line by line
      // at the output stage.
    }

    fseek($docfile, 0);

    $header = array();
    $rows = array();

    // Generate Header
    // For fields with a single value, the header is simply wrapped.
    // For fields where at least one multiple value was found across
    // the resultset, the field is iterated the maximum number of times.
    foreach ($field_counts as $field => $count) {
      $label = isset($result_fields[$field]) ?
        $result_fields[$field] :
        $field;
      if ($count == 1) {
        $header[] = $wrap . str_replace('"', $replace_value, $label) . $wrap;
      }
      else {
        $i = 1;
        while ($i <= $count) {
          $header[] = $wrap . str_replace('"', $replace_value, $label . "_" . $i) . $wrap;
          $i++;
        }
      }
    }

    // Generate refactored output matrix.
    $row_count = 0;
    while ($line = fgets($docfile)) {
      $doc = unserialize(stripcslashes($line));
      foreach ($field_counts as $field => $count) {
        $i = 0;
        // @todo Make it write to another file, and output the file.
        while ($i < $count) {
          if (isset($doc[$field]['value'])) {
            if ($i === 0 && is_string($doc[$field]['value'])) {
              $rows[$row_count][] = $wrap . str_replace('"', $replace_value, $doc[$field]['value']) . $wrap;
            }
            elseif (isset($doc[$field][$i])) {
              $rows[$row_count][] = $wrap . str_replace('"', $replace_value, $doc[$field]['value'][$i]) . $wrap;
            }
            else {
              $rows[$row_count][] = "";
            }
          }
          else {
            $rows[$row_count][] = "";
          }
          $i++;
        }
      }
      $row_count++;
    }

    fclose($docfile);

    drupal_add_http_header('Content-Type', 'text/csv; utf-8');
    drupal_add_http_header('Content-Disposition', 'attachment; filename="searchresults.csv');

    print implode($seperator, $header) . "\r\n";
    foreach ($rows as $row) {
      print implode($seperator, $row) . "\r\n";
    }

    drupal_set_message(format_plural(
      count($rows),
      'Exported 1 record from the index to CSV.',
      'Exported @count records from the index to CSV.'
    ));

    drupal_exit();
  }

}
