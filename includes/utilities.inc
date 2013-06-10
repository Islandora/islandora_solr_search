<?php

/**
 * @file
 * Miscellaneous helper functions.
 */

/**
 * Initialize a pager for theme('pager') without running an SQL query.
 *
 * The nuts and bolts of this function are borrowed from z3950.module
 * and also apachesolr.module.
 *
 * @see pager_query()
 *
 * @param int $total
 *   The total number of items found.
 * @param int $limit
 *   The number of items you will display per page.
 * @param int $element
 *   An optional integer to distinguish between multiple pagers on one page.
 *
 * @return int
 *   The current page for $element. 0 by default if $_GET['page'] is empty.
 */
function islandora_solr_pager_init($total, $limit = 10, $element = 0) {
  global $pager_page_array, $pager_total, $pager_total_items;
  $page = isset($_GET['page']) ? $_GET['page'] : '';

  // Convert comma-separated $page to an array, used by other functions.
  $pager_page_array = explode(',', $page);

  // We calculate the total of pages as ceil(items / limit).
  $pager_total_items[$element] = $total;
  $pager_total[$element] = ceil($pager_total_items[$element] / $limit);
  $pager_page_array[$element] = max(0, min((int) $pager_page_array[$element], ((int) $pager_total[$element]) - 1));

  // Return the current position.
  return $pager_page_array[$element];
}

/**
 * Restore slashes in a string.
 *
 * Can't have a slash in the URL, as it will break Drupal clean URLs.
 *
 * @param string $str
 *   An arbitrary string.
 *
 * @return string
 *   The string with any occurences of '~slsh~' and '~plus~' changed to '/' and
 *   '+' respectively... Effectively the opposite of
 *   islandora_solr_replace_slashes().
 *
 * @see islandora_solr_replace_slashes()
 */
function islandora_solr_restore_slashes($str) {
  $replacements = array("~slsh~", "~plus~");
  $values = array("/", "+");
  return str_replace($replacements, $values, $str);
}

/**
 * Replace slashes in a string.
 *
 * Can't have a slash in the URL, as it will break Drupal clean URLs.
 *
 * @param string $str
 *   An arbitrary string.
 *
 * @return string
 *   The string with any occurences of '/' and '+' changed to '~slsh~' and
 *   '~plus~' respectively... Effectively the opposite of
 *   islandora_solr_restore_slashes().
 *
 * @see islandora_solr_restore_slashes()
 */
function islandora_solr_replace_slashes($str) {
  $replacements = array("~slsh~", "~plus~");
  $values = array("/", "+");
  return str_replace($values, $replacements, $str);
}

/**
 * A better explode method allows quotes in the returned strings.
 *
 * Taken from php.net.
 *
 * @param string $delim
 *   The delimiter on which to break (',' by default).
 * @param string $str
 *   The string to break.
 * @param string $enclose
 *   An enclosing character.
 * @param bool $preserve
 *   ??? Something to do with $enclose.
 *
 * XXX: Is this function really necessary?
 *
 * @return array
 *   An array containing the items of $str, broken apart on $delim.
 */
function islandora_solr_csv_explode($delim = ',', $str = '', $enclose = '"', $preserve = FALSE) {
  $res_arr = array();
  $n = 0;
  $exp_enc_arr = explode($enclose, $str);
  foreach ($exp_enc_arr as $enc_item) {
    if ($n++ % 2) {
      array_push($res_arr, array_pop($res_arr) . ($preserve ? $enclose : '') . $enc_item . ($preserve ? $enclose : ''));
    }
    else {
      $exp_del_arr = explode($delim, $enc_item);
      array_push($res_arr, array_pop($res_arr) . array_shift($exp_del_arr));
      $res_arr = array_merge($res_arr, $exp_del_arr);
    }
  }
  return $res_arr;
}

/**
 * Escape a string for inclusion in a query.
 *
 * Copied from Apache_Solr_Service and added space as a value to escape.
 *
 * List of characters taken from
 * @link http://lucene.apache.org/java/docs/queryparsersyntax.html#Escaping%20Special%20Characters Lucene documentation. @endlink
 *
 * Divergences from Apache_Solr_Service:
 * - remove '?' and whitespace (Zac):
 *   - '?' is useful.
 *   - Escaping whitespace is unnecessary.
 * - remove '*' (Alan):
 *   - Allows wildcard searches.
 *
 * @param string $value
 *   A string to escape.
 *
 * @return string
 *   The escaped string.
 */
function islandora_solr_lesser_escape($value) {
  $pattern = '/(\+|-|&&|\|\||!|\(|\)|\{|}|\[|]|\^|~|\?|:|\\\)/';
  $replace = '\\\$1';

  return preg_replace($pattern, $replace, $value);
}

/**
 * Escape characters in field names of facets.
 */
function islandora_solr_escape($facets) {
  $return_facets = array();
  foreach ($facets as $facet) {
    $tmp = drupal_substr($facet, 0, strpos($facet, ':"'));
    $tmp = lesser_escape(trim($tmp));

    $return_facets[] = $tmp . drupal_substr($facet, strpos($facet, ':"'), drupal_strlen($facet));
  }
  return $return_facets;
}

/**
 * Turns term fields strings from the admin forms into arrays.
 *
 * Mostly used with variable_get().
 *
 * @deprecated
 *   No longer used, due to the use of a separate database table for
 *   configuration.
 *
 * @param string $raw_field_list
 *   A string containing the list of fields to parse, with individual items
 *   separated by either a comma or newline. Each field entry may be either a
 *   field name alone, or a field name, a tilde, and then a human-readable
 *   name, so for the field "field_name":
 *   - "field_name": The field itself will be used as the "human-readable".
 *   - "field_name ~ My Cool Field": "My Cool Field" will be used as as the
 *     human-readable version.
 *
 * @return array
 *   An associative array mapping the fields to our human-readable label.
 */
function islandora_solr_build_substitution_list($raw_field_list) {
  $facet_array = array();
  $facet_fields = '';
  $lines = preg_split('/[,|\n|\r]/', $raw_field_list);
  foreach ($lines as $line) {
    if ($line) {
      $line_array = explode('~', $line);
      $key = trim($line_array[0]);
      if (count($line_array) > 1) {
        $value = trim($line_array[1]);
      }
      else {
        $value = $key;
      }
      $facet_array[$key] = $value;
      if ($facet_fields) {
        $facet_fields .= ",";
      }
      $facet_fields .= $key;
    }
  }
  return $facet_array;
}

/**
 * Prepares the Solr docs of an object results array.
 *
 * Preparing the Solr doc array includes adding the label, value and class name
 * based on the Solr field name. When you created a new Solr display, it is
 * not required to use this helper function, but it can be helpful.
 *
 * @param array $object_results
 *   An array containing the object results. This includes the Solr doc array.
 *
 * @return array
 *   The original object results array, but with a modified Solr doc.
 */
function islandora_solr_prepare_solr_doc($object_results) {
  $fields_all = islandora_solr_get_fields('result_fields', FALSE);
  // Loop over object results.
  foreach ($object_results as $object_index => $object_result) {
    $doc = $object_result['solr_doc'];
    $rows = array();
    foreach ($doc as $field => $value) {
      if (isset($fields_all[$field])) {
        $label = $fields_all[$field];
      }
      else {
        $label = $field;
      }
      $rows[$field]['label'] = $label;
      $rows[$field]['class'] = drupal_strtolower(preg_replace('/[^A-Za-z0-9]/', '-', $field));
      $rows[$field]['value'] = $value;
    }
    // Replace Solr doc.
    $object_results[$object_index]['solr_doc'] = $rows;
  }
  return $object_results;
}

/**
 * Prepares Solr results before rendering.
 *
 * Prepares Solr results before rendering. Applies highlighting, implodes
 * arrays and links to objects to Solr result fields.
 *
 * @param array $solr_results
 *   Array containing the Solr results which are altered trough the query
 *   processor.
 *
 * @return array
 *   Returns the same array but with prepared Solr field values.
 */
function islandora_solr_prepare_solr_results($solr_results) {
  $object_results = $solr_results['response']['objects'];
  $highlighting = isset($solr_results['highlighting']) ? $solr_results['highlighting'] : array();
  $fields_all = islandora_solr_get_fields('result_fields', FALSE);
  $link_to_object = islandora_solr_get_link_to_object_fields();
  $date_format = islandora_solr_get_date_format_fields();

  // Loop over object results.
  foreach ($object_results as $object_index => $object_result) {
    $solr_doc = $object_result['solr_doc'];
    $pid = $object_result['PID'];
    $rows = array();
    $options = array('html' => TRUE);
    if (isset($object_result['object_label'])) {
      $options['attributes']['title'] = $object_result['object_label'];
    }
    if (isset($object_result['object_url_params'])) {
      $options['query'] = $object_result['object_url_params'];
    }
    if (isset($object_result['object_url_fragment'])) {
      $options['fragment'] = $object_result['object_url_fragment'];
    }

    foreach ($solr_doc as $field => $value) {
      // Date formatting.
      if (isset($date_format[$field])) {
        $value = format_date(strtotime($value), 'custom', $date_format[$field], 'UTC');
      }
      // Only apply highlighting when the date isn't formatted.
      else {
        // Check highlighting.
        if (isset($highlighting[$pid][$field])) {
          $value = $highlighting[$pid][$field];
        }
        else {
          $value = $solr_doc[$field];
        }
      }
      // Implode.
      $value = is_array($value) ? implode(", ", $value) : $value;
      // Add link.
      if (in_array($field, $link_to_object)) {
        $value = l($value, $object_result['object_url'], $options);
      }
      $solr_doc[$field] = $value;
    }
    // Replace Solr doc rows.
    $solr_results['response']['objects'][$object_index]['solr_doc'] = $solr_doc;
  }
  return $solr_results;
}

/**
 * @defgroup islandora_solr_deprecated Deprecated function signatures.
 * @{
 * A number of function signatures which are not prefixed with our module name.
 *
 * Supporting them with a warning for now, just in case.
 */

// @codingStandardsIgnoreStart

/**
 * Old function signature.
 *
 * @deprecated
 *   Use islandora_solr_build_substitution_list().
 * @see islandora_solr_build_substitution_list()
 */
function islandora_build_substitution_list($value) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $message = islandora_deprecated('7.x-1.2', t('Refactor to use islandora_solr_build_substitution_list().'));
  trigger_error(filter_xss($message), E_USER_DEPRECATED);

  return islandora_solr_build_substitution_list($value);
}

/**
 * Old function signature.
 *
 * @deprecated
 *   Use islandora_solr_escape().
 * @see islandora_solr_escape()
 */
function solr_escape($facets) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $message = islandora_deprecated('7.x-1.2', t('Refactor to use islandora_solr_escape().'));
  trigger_error(filter_xss($message), E_USER_DEPRECATED);

  return islandora_solr_escape($facets);
}

/**
 * Old function signature.
 *
 * @deprecated
 *   Use islandora_solr_lesser_escape().
 * @see islandora_solr_lesser_escape()
 */
function lesser_escape($value) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $message = islandora_deprecated('7.x-1.2', t('Refactor to use islandora_solr_lesser_escape().'));
  trigger_error(filter_xss($message), E_USER_DEPRECATED);

  return islandora_solr_lesser_escape($value);
}

/**
 * Old function signature.
 *
 * @deprecated
 *   Use islandora_solr_csv_explode().
 * @see islandora_solr_csv_explode()
 */
function csv_explode($value) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $message = islandora_deprecated('7.x-1.2', t('Refactor to use islandora_solr_csv_explode().'));
  trigger_error(filter_xss($message), E_USER_DEPRECATED);

  return islandora_solr_csv_explode($value);
}

/**
 * Old function signature.
 *
 * @deprecated
 *   Use islandora_solr_restore_slashes().
 * @see islandora_solr_restore_slashes()
 */
function restore_slashes($value) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $message = islandora_deprecated('7.x-1.2', t('Refactor to use islandora_solr_restore_slashes().'));
  trigger_error(filter_xss($message), E_USER_DEPRECATED);

  return islandora_solr_restore_slashes($value);
}

/**
 * Old function signature.
 *
 * @deprecated
 *   Use islandora_solr_replace_slashes().
 * @see islandora_solr_replace_slashes()
 */
function replace_slashes($value) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $message = islandora_deprecated('7.x-1.2', t('Refactor to use islandora_solr_replace_slashes().'));
  trigger_error(filter_xss($message), E_USER_DEPRECATED);

  return islandora_solr_replace_slashes($value);
}

// @codingStandardsIgnoreEnd

/**
 * @} End of "defgroup islandora_solr_deprecated".
 */
