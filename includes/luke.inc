<?php

/**
 * @file
 * Contains helper functions for Solr Luke.
 */

/**
 * Pings a Solr instance for availability.
 *
 * @param string $solr_url
 *   A URL that points to Solr.
 *
 * @return int|bool
 *   Returns ping time in milliseconds on success or boolean FALSE if Solr
 *   could not be reached.
 */
function islandora_solr_ping($solr_url) {
  // Check that scheme specifier is present.
  $solr_url = islandora_solr_check_http($solr_url);
  // Check for valid URL string.
  if (!filter_var($solr_url, FILTER_VALIDATE_URL)) {
    return FALSE;
  }

  $solr_url_parsed = parse_url($solr_url);
  // If it's not a correct URL for Solr to check, return FALSE.
  if (!isset($solr_url_parsed['host']) || !isset($solr_url_parsed['port'])) {
    return FALSE;
  }
  // Call Solr.
  $solr_service = new Apache_Solr_Service($solr_url_parsed['host'], $solr_url_parsed['port'], $solr_url_parsed['path'] . '/');
  // Ping Solr.
  $ping = $solr_service->ping();
  // If a ping time is returned.
  if ($ping) {
    // Add 0.1 ms to the ping time so we never return 0.0.
    return $ping + 0.01;
  }
  return FALSE;
}

/**
 * Check for URL scheme.
 *
 * @param string $url
 *   A URL to check.
 *
 * @return string
 *   If the url doesn't include a scheme ('http://') then it will
 *   include the url with added 'http://' scheme.
 */
function islandora_solr_check_http($url) {
  // Check for 'http://' prefix - if not found, add "http://".
  if (strpos($url, "http://") === FALSE) {
    $url = 'http://' . $url;
  }
  return $url;
}

/**
 * Get Luke info.
 *
 * @param string $solr_url
 *   URL which points to Solr.
 * @param string $field
 *   Limit luke results by entering a single Solr field.
 * @param int $num_terms
 *   Number of terms to include per Solr field. Terms are indexed strings which
 *   include the number of occurrences in the Solr index. Higher occurrences are
 *   listed first. Defaults to 0 because including this can add a siginificant
 *   performance hit.
 * @param bool $show_schema
 *   Get the schema information as part of this request. WARNING: Enabling this
 *   will override a field display and ONLY display the schema information.
 *
 * @return array|bool
 *   Returns Solr Luke data, or boolean FALSE if there was an error (Solr not
 *   available?).
 */
function islandora_solr_get_luke($solr_url = NULL, $field = NULL, $num_terms = 0, $show_schema = FALSE) {
  if (!$solr_url) {
    // Create URL.
    $solr_url = variable_get('islandora_solr_url', 'localhost:8080/solr');
  }
  // If Solr is available get Luke.
  if (islandora_solr_ping($solr_url)) {
    // Create URL.
    $luke_url = $solr_url . '/admin/luke';
    // Check for scheme specifier.
    $luke_url = islandora_solr_check_http($luke_url);
    $luke_query = array(
      'wt' => 'json',
      'numTerms' => $num_terms,
    );
    if ($field) {
      $luke_query['fl'] = $field;

      // XXX: Solr 4 breaks when specifying numTerms=0 (and a field), see
      // https://issues.apache.org/jira/browse/SOLR-5107
      if ($luke_query['numTerms'] == 0) {
        $luke_query['numTerms'] = 1;
      }
    }
    if ($show_schema) {
      $luke_query['show'] = 'schema';
    }
    // Generate nice URL.
    $luke_url = url($luke_url, array('absolute' => TRUE, 'query' => $luke_query));
    // Need to set a timeout greater than the default 30 seconds as some indexes
    // could contain a lot of fields and the Luke needs longer to respond in
    // kind.
    $timeout = variable_get('islandora_solr_luke_timeout', '45');
    $r = drupal_http_request($luke_url, array('timeout' => $timeout));
    if ($r->code / 100 === 2) {
      $luke_json = $r->data;
      // Parse JSON.
      $luke_array = json_decode($luke_json, TRUE);
      return $luke_array;
    }
    else {
      return FALSE;
    }
  }
  else {
    return FALSE;
  }
}

/**
 * Returns an array containing all fields in Luke that are sortable.
 *
 * Must be indexed and can't be multivalued.
 * @link http://archive.apache.org/dist/lucene/solr/ref-guide/
 * Common Query Parameters: Sort section for more info on constraints. @endlink
 *
 * @param string $solr_url
 *   URL which points to Solr.
 *
 * @return array|bool
 *   An array containing all fields in Luke that are sortable. If no
 *   fields are sortable, it returns FALSE.
 */
function islandora_solr_get_sortable($solr_url = NULL) {
  $sortable = array();
  $luke = islandora_solr_get_luke($solr_url);
  // Check if fields are available.
  if (isset($luke['fields']) && !empty($luke['fields'])) {
    foreach ($luke['fields'] as $field => $value) {
      // Check if field is sortable.
      if (strstr($value['schema'], "I") != FALSE && strstr($value['schema'], "M") == FALSE) {
        $sortable[] = $field;
      }
    }
  }
  // Return FALSE if no sortable fields are returned.
  if (empty($sortable)) {
    return FALSE;
  }
  return $sortable;
}

/**
 * Checks a single Solr field if it's sortable.
 *
 * Must be indexed and can't be multivalued.
 * @link http://wiki.apache.org/solr/CommonQueryParameters#sort More info on constraints. @endlink
 *
 * @param string $solr_url
 *   URL which points to Solr.
 * @param string $field
 *   Solr field to be checked for being sortable.
 *
 * @return bool
 *   Returns TRUE if the given field is sortable, returns FALSE if the field is
 *   not sortable or if field is not set.
 */
function islandora_solr_check_sortable($solr_url = NULL, $field = NULL) {
  $sortable = FALSE;

  if ($field) {
    $luke = islandora_solr_get_luke($solr_url, $field);
    // Check if fields are available.
    if (!empty($luke['fields'])) {
      $schema = $luke['fields'][$field]['schema'];
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
 *
 * @return bool
 *   Returns TRUE if the Solr field is both tokenized and stored.
 */
function islandora_solr_check_highlighting_allowed($solr_field = NULL) {
  // Can't find proper documentation on which field types included in
  // highlighting breaks Solr and what types don't. For now add them to this
  // array.
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

/**
 * Function to get the actual class for a type.
 *
 * @param string $solr_type
 *   Solr type to get the class for.
 *
 * @return string
 *   The classname of the type.
 */
function islandora_solr_get_type_class($solr_type) {
  // Get the schema information.
  $luke_result = islandora_solr_get_luke(NULL, NULL, 0, TRUE);
  if (isset($luke_result['schema']['types'][$solr_type])) {
    return $luke_result['schema']['types'][$solr_type]['className'];
  }
  else {
    return FALSE;
  }
}

/**
 * Determine the version of solr being run.
 *
 * @param bool $reset
 *   Force a reset of the static version variable.
 *
 * @return array|bool
 *   The version array or FALSE if can't be determined.
 *
 * Version array has keys:
 *    'version' => 'string of version'.
 *    'major' => first digit (if parseable),
 *    'minor' => second digit (if applicable),
 *    'patch' => third digit (if applicable)
 *
 *  Only version is guaranteed to be returned on a successful request.
 */
function islandora_solr_get_solr_version($reset = FALSE) {
  $version = &drupal_static(__FUNCTION__);
  if (!isset($version) || $reset) {
    $version = FALSE;
    $solr_url = variable_get('islandora_solr_url', 'localhost:8080/solr');
    if (islandora_solr_ping($solr_url)) {
      $parts = explode('/', $solr_url);
      while (count($parts) >= 2) {
        // If we pass a core, we need to move up to solr root.
        $solr_url = implode('/', $parts);
        $admin_url = $solr_url . '/admin/info/system';
        $admin_url = islandora_solr_check_http($admin_url);
        // Need to set a timeout greater than the default 30 seconds as some
        // indexescould contain a lot of fields and the Luke needs longer to
        // respond in kind.
        $timeout = variable_get('islandora_solr_luke_timeout', '45');
        $admin_url = url($admin_url, array(
          'absolute' => TRUE,
          'timeout' => $timeout,
          'query' => array(
            'wt' => 'json',
          ),
        ));
        $r = drupal_http_request($admin_url);
        if ($r->code / 100 === 2) {
          $admin_json = $r->data;
          // Parse JSON.
          $admin_array = json_decode($admin_json, TRUE);
          if (isset($admin_array['lucene']['solr-spec-version'])) {
            $v_parts = explode('.', $admin_array['lucene']['solr-spec-version']);
            $version = array(
              'version' => $admin_array['lucene']['solr-spec-version'],
            );
            if ($v_parts) {
              $version['major'] = (int) $v_parts[0];
            }
            if (count($v_parts) > 1) {
              $version['minor'] = (int) $v_parts[1];
            }
            if (count($v_parts) > 2) {
              $version['patch'] = (int) $v_parts[2];
            }
            break;
          }
        }
        array_pop($parts);
      }
    }
  }
  return $version;
}

/**
 * Returns an array of schema markers to define what is set for the field.
 *
 * Keys are built from Solr so could change based on Solr version. But so far
 * seem consistent (made lowercase):
 * - indexed
 * - tokenized
 * - stored
 * - multivalued
 * - termvector stored
 * - store offset with termvector
 * - store position with termvector
 * - omit norms
 * - lazy
 * - binary
 * - sort missing first
 * - sort missing last
 *
 * @param $field_name
 *   The field to check
 *
 * @return array
 *   Array with true for schema
 *
 * @see https://cwiki.apache.org/confluence/display/solr/LukeRequestHandler
 */
function islandora_solr_get_field_schema($field_name) {
  $info = islandora_solr_get_luke(NULL, $field_name, 1);
  $field_schema = array();
  if (is_array($info)) {
    if (isset($info['fields'][$field_name]['schema']) && isset($info['info']['key'])) {
      // Only test for active values, ie. non hyphens.
      $encoded_schema = str_replace('-', '', $info['fields'][$field_name]['schema']);
      // Get the schema key from Luke
      $schema_key = $info['info']['key'];
      // Turn values to lowercase
      array_walk($schema_key, function(&$o) { $o = strtolower($o);});
      // Make array with keys from schema_key and values all False
      $field_schema = array_combine(array_values($schema_key), array_pad(array(), count($schema_key), FALSE));
      for ($foo = 0; $foo < strlen($encoded_schema); $foo += 1) {
        $key = substr($encoded_schema, $foo, 1);
        if (array_key_exists($key, $schema_key)) {
          $field_schema[$schema_key[$key]] = TRUE;
        }
      }
    }
  }
  return $field_schema;
}
