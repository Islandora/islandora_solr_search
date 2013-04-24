<?php

/**
 * @file
 * Contains helper functions for Solr Luke.
 */

/**
 * Pings a solr instance for availability.
 *
 * @param $solr_url
 *   A url that points to Solr.
 * @return
 *   Returns ping time in milliseconds or a FALSE boolean
 */
function islandora_solr_ping($solr_url) {
  // check for 'http://' or 'https://' prefix
  $solr_url = islandora_solr_check_http($solr_url);
  // check for valid url string.
  if (!filter_var($solr_url, FILTER_VALIDATE_URL)) {
    return FALSE;
  }
  // split url in host/port/path
  $solr_url_parsed = parse_url($solr_url);
  // if it's not a correct url for solr to check, return false.
  if (!isset($solr_url_parsed['host']) || !isset($solr_url_parsed['port'])) {
    return FALSE;
  }
  // call solr
  $solr_service = new Apache_Solr_Service($solr_url_parsed['host'], $solr_url_parsed['port'], $solr_url_parsed['path'] . '/');
  // ping solr
  $ping = $solr_service->ping();
  // if a ping time is returned
  if ($ping) {
    // Add 0.1 ms to the ping time so we never return 0.0.
    return $ping + 0.01;
  }
  return FALSE;
}

/**
 * Check for url scheme
 *
 * @param string $url
 *   A url to check
 * @return string
 *   If the url doesn't include a scheme ('http://' or 'https://') then it will
 *   include the url with added 'http://' scheme.
 */
function islandora_solr_check_http($url) {
  // check for 'http://' or 'https://' prefix - if not found, add it.
  if (strpos($url, "http://") === FALSE && strpos($url, "https://") === FALSE) {
    $url = 'http://' . $url;
  }
  return $url;
}

/**
 * Function to get Solr luke as a php array.
 *
 * @param string $solr_url
 *   url which points to solr
 * @param string $field
 *   limit luke results by entering a single solr field
 * @param int $num_terms
 *   number of terms to include per solr field. Terms are indexed strings which
 *   include the number of occurrences in the solr index. Higher occurrences are
 *   listed first. Defaults to 0 because including this can add a siginificant
 *   perfomance hit.
 * @return
 *   if solr is available it returns Solr Luke data as a php array, if not it
 *   returns FALSE.
 */
function islandora_solr_get_luke($solr_url = NULL, $field = NULL, $num_terms = 0) {
  if (!$solr_url) {
    // create url
    $solr_url = variable_get('islandora_solr_url', 'localhost:8080/solr');
  }
  // if solr is available get luke
  if (islandora_solr_ping($solr_url)) {
    // create url
    $luke_url = $solr_url . '/admin/luke';
    // check for http://
    $luke_url = islandora_solr_check_http($luke_url);
    $luke_query = array(
      'wt' => 'json',
      'numTerms' => $num_terms,
    );
    if ($field) {
      $luke_query['fl'] = $field;
    }
    // generate nice url
    $luke_url = url($luke_url, array('absolute' => TRUE, 'query' => $luke_query));
    // get file content (= json string)
    $luke_json = file_get_contents($luke_url, 0, NULL, NULL);
    // decode json to php array
    $luke_array = json_decode($luke_json, TRUE);
    return $luke_array;
  }
  else {
    return FALSE;
  }
}

/**
 * Returns an array containing all fields in Luke that are sortable.
 *
 * @param string $solr_url
 *   url which points to solr
 * @return boolean
 *   returns an array containing all fields in Luke that are sortable. If no
 *   fields are sortable, it returns FALSE
 */
function islandora_solr_get_sortable($solr_url = NULL) {
  // set variable
  $sortable = array();
  $luke = islandora_solr_get_luke($solr_url);
  // check if fields are available
  if (isset($luke['fields']) && !empty($luke['fields'])) {
    // loop over fields
    foreach ($luke['fields'] as $field => $value) {
      // scheck if field is sortable
      // must be indexed and can't be multivalued
      // http://wiki.apache.org/solr/CommonQueryParameters#sort
      if (strstr($value['schema'], "I") != FALSE && strstr($value['schema'], "M") == FALSE) {
        // add field
        $sortable[] = $field;
      }

    }
  }
  // return FALSE if no sortable fields are returned
  if (empty($sortable)) {
    return  FALSE;
  }
  return $sortable;
}

/**
 * Checks a single solr field if it's sortable.
 *
 * @param string $solr_url
 *   url which points to solr
 * @param string $field
 *   solr field to be checked for being sortable
 * @return boolean
 *   Returns TRUE if the given field is sortable, returns FALSE if the field is
 *   not sortable or if field is not set.
 */
function islandora_solr_check_sortable($solr_url = NULL, $field = NULL) {
  // set variable
  $sortable = FALSE;

  if ($field) {
    $luke = islandora_solr_get_luke($solr_url, $field);
    // check if fields are available
    if (!empty($luke['fields'])) {
      $schema = $luke['fields'][$field]['schema'];
      // scheck if field is sortable
      // must be indexed and can't be multivalued
      // http://wiki.apache.org/solr/CommonQueryParameters#sort
      if (strstr($schema, "I") != FALSE && strstr($schema, "M") == FALSE) {
        $sortable = TRUE;
      }
    }
  }
  return $sortable;
}

/**
 * Function to check if highlighting is allowed for a given Solr field.
 *
 * @param string $solr_field
 *   Solr field to check if highlighting is allowed.
 * @return bool
 *   Returns TRUE if the Solr field is both tokenized and stored.
 */
function islandora_solr_check_highlighting_allowed($solr_field = NULL) {
  // Can't find proper documentation on which field types included in highlighting breaks Solr and what
  // types don't. For now add them to this array.
  $types_breaking_solr = array('date');
  $luke_field = islandora_solr_get_luke(NULL, $solr_field);
  if (isset($luke_field['fields'][$solr_field]['schema']) && isset($luke_field['fields'][$solr_field]['type'])) {
    $schema = $luke_field['fields'][$solr_field]['schema'];
    $type = $luke_field['fields'][$solr_field]['type'];
    if (strpos($schema, 'I') !== FALSE && strpos($schema, 'S') !== FALSE && !in_array($type, $types_breaking_solr)) {
      return TRUE;
    }
  }
  return FALSE;
}
