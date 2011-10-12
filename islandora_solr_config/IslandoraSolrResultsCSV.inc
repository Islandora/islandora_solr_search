<?php

/*
 * contains methods to search solr and display results.  depends on Apache_Solr_Php client.
 */

module_load_include('inc', 'islandora_solr_search', 'IslandoraSolrResults');

/**
 * Extention of IslandoraSolrResults for templating purposes.
 * This overrides the displayResults function to provide an alternate display type.
 */

class IslandoraSolrResultsCSV extends IslandoraSolrResults {

  function IslandoraSolrResultsCSV() {
    module_load_include('php', 'islandora_solr_search', 'SolrPhpClient/Apache/Solr/Service');
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
  function printCSV($solrQueryProcessor, $title = "Search Results") {

    // Note: solrLimit is really stating the number of rows wanted,
    // not the row number of the upper limit document.  That is, if you
    // want results 40-60, you set solrStart=40, solrLimit=20 -- *not*
    // solrStart=40, solrLimit=60.

    global $base_url;

    // First off, update outer limits
    $upperLimit = $solrQueryProcessor->solrResult->response->numFound;
    $lowerLimit = 0;
    $increment = 5000;
    // typical failure point for an unbounded query seems to be around 10000-15000,
    // but we must allow for the every-growing output memory structure.

    $row_count = 0;
    $field_counts = array();
    $values = array();

    $seperator = ",";
    $wrap = '"';
    $replace_value = '""';

    $solrQueryProcessor->solrStart = $lowerLimit;
    $solrQueryProcessor->solrLimit = $increment;

    $docfile = tmpfile();

    while( $solrQueryProcessor->solrStart < $upperLimit ) {

      // Perform the incremental re-query.
      $solrQueryProcessor->resetResults();
      $solrQueryProcessor->executeQuery();

      // Update incremental limits for the next round.
      $lowerLimit += $increment;
      $solrQueryProcessor->solrStart = $lowerLimit;

      // If there are no results, skip ahead (i.e. exit).
      if(empty($solrQueryProcessor->solrResult)) {
        continue;
      }

      // else ...
      $rawResponse = $solrQueryProcessor->solrResult->getRawResponse();
      $responseData = json_decode($rawResponse,TRUE); // TRUE == associative array, not object
      unset($rawResponse); // unknown usefulness

      $docs = $responseData['response']['docs'];

      // This loop is just to update the maximum value counts for each field
      // because if any field in the entire resultset has multiple values,
      // we must break the field into multiple iterated fields.
      foreach($docs as $doc) {
        fputs( $docfile, addcslashes( serialize($doc), "\n\r\\" )."\n");
        foreach($doc as $field => $value) {

          $field_count = is_array($value) ? count($value) : 1;

          if( !isset( $field_counts[$field] )) {
            $field_counts[$field] = $field_count;
          }
          else {
            $field_counts[$field] = max($field_counts[$field], $field_count);
          }

          unset($value);
          unset($field);
        }
        $row_count++;
        unset($doc); // unknown usefulness
        // I have read that PHP sometimes retains loop variables out of scope,
        // so I'm manually unsetting them with each iteration.
      }
      unset($docs);
      unset($responseData); // unknown usefulness

      // Let's try writing a serialized version of the each doc to a file and removing it from
      // memory, then digging through said file line by line at the output stage.

    }

    fseek($docfile,0);

    $header = array();
    $rows = array();

    // Generate Header
    // For fields with a single value, the header is simply wrapped.
    // For fields where at least one multiple value was found across
    // the resultset, the field is iterated the maximum number of times.
    foreach( $field_counts as $field=>$count ) {
      if( $count == 1 ) {
        $header[] = $wrap . str_replace('"', $replace_value, $field) . $wrap;
      }
      else {
        $i = 1;
        while( $i <= $count ) {
          $header[] = $wrap . str_replace('"', $replace_value, $field."_".$i) . $wrap;
          $i++;
        }
      }
    }

    // Generate refactored output matrix.
    $row_count = 0;
    while( $line = fgets( $docfile )) {
      $doc = unserialize( stripcslashes( $line ));
      foreach( $field_counts as $field=>$count ) {
        $i = 0;
        while( $i < $count ) {
          if( isset( $doc[$field] )) {
            if( $i === 0 && is_string($doc[$field] )) {
              $rows[$row_count][] = $wrap . str_replace('"', $replace_value, $doc[$field]) . $wrap;
            }
            elseif( isset( $doc[$field][$i] ) ) {
              $rows[$row_count][] = $wrap . str_replace('"', $replace_value, $doc[$field][$i]) . $wrap;
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

    drupal_set_header('Content-Type: text/csv');
    drupal_set_header('Content-Disposition: attachment; filename="searchresults.csv"');

    print implode($seperator, $header) . "\r\n";
    foreach( $rows as $count => $row ) {
      print implode($seperator, $row) . "\r\n";
    }

    drupal_set_message("Exported ".($count+1)." records from index to CSV");

    fclose($docfile);
    
    exit;

  }

}

