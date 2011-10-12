<?php

/*
 * contains methods to search solr and display results.  depends on Apache_Solr_Php client.
 */

module_load_include('inc', 'islandora_solr_search', 'IslandoraSolrResults');

/**
 * Extention of IslandoraSolrResults for templating purposes.
 * This overrides the displayResults function to provide an alternate display type.
 */

class IslandoraSolrResultsTable extends IslandoraSolrResults {

  static $facetSeparator = '~';//used to separate facets in url
  static $slashReplacement = '~slsh~';// a pattern used to replace / in url's the slash breaks drupal clean url's
  public static $SEARCH_CLASS_ADVANCED_SEARCH_NUMBER_FIELDS = 5;
  function IslandoraSolrResultsTable() {
    parent::__construct();
  }

  /**
   * Outputs results in unified tabular format.
   *
   * This function collects all field names from all results in the 
   * resultset and constructs a single table with said field names in 
   * a unified header row, rather than outputting each result as a 
   * miniature horizontal table.
   *
   * @param $results
   *   A solr resultset object.
   *
   * @return
   *   html output for the resultset. Note: we currently create this 
   *   output manually, should refactor to use drupal forms api.
   */
  function printResults($results){
    global $base_url;
    $recordStart = (int)$results->response->start;
    if(empty($results)) {
      return "no results";
    }
    $count = 0;
    $fields = array();
    $values = array();
    $header = array();
    $limitResults = variable_get('islandora_solr_search_limit_result_fields', 0);

    $object_link_placeholder = "~object_href~";

    foreach($results->response->docs as $doc) {
      unset($pid);
      foreach($doc as $field => $value) {

        if( $limitResults && empty($this->resultFieldArray[$field] ) ) {
          continue;
        }

        // catches the object PID.
        if($field=='PID') {
          $pid = $value;
          // we need this in the resultset, but would prefer to hide it from users.
        }

        // Sometimes we get a single value, sometimes an array.  
        // For simplicity, I push the singles into an array and later on process each
        // field as an array.
        if(!is_array($value)) {
          $value = array($value);
        }

        // We're going to put the object link onto the dublin-core title.
        if($field=='dc.title' || $field=='dc:title') {
          foreach ($value as $key=>$val) {
            if(isset($doc->PID)) {
              $value[$key] = l($val,'fedora/repository/'.$doc->PID.'/-/'.$val);
            }
          }
        }
        $value = implode(", ",$value);

        //this would change 'animal_name' to the more readable 'Animal Name'
        //$field = ucwords( str_replace( '_', ' ', $field ) );

        // Add this field to the master list of fields if it's not there already:
        if (!in_array( $field, $fields )) {
          $fields[] = $field;
        }

        $values[$count][$field] = $value;
      }
      $count++;
    }

    foreach( $fields as $field ) {
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
    array_unshift($header,'#');
    
    $rows = array();
    $row = 0;

    foreach ($values as $num => $val_array) {
      unset($rowclass);
      $result_num = $recordStart + $num + 1;
      $rows[$row][] = array(
        'data' => (isset($val_array['PID']) ? l($result_num,'fedora/repository/'.$val_array['PID']) : $result_num),
        'header' => TRUE,
      );
      foreach ($fields as $field) {
        if (isset ($val_array[$field])) {
          $rows[$row][] = $val_array[$field];
        } else {
          $rows[$row][] = '';
        }
      }
      $row++;
    }

    $output .= theme_table($header,$rows,array('class'=>'islandora_solr_search_results_table'));

    if( variable_get('islandora_solr_search_debug_mode', 0) ) { // debug dump
      $results_r .= "<pre>Results: ".print_r($results,TRUE)."</pre>";
      $fieldset_r = array(
        '#title' => t("Raw Results"),                                                                                                                                             
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#value' => $results_r,
        );
      $output .= theme('fieldset', $fieldset_r);
    }
    return $output;
  }

}

