<?php

/*
 * @file
 * Contains some non-class-specific methods and things I'm not sure where to put.
 */

$values = array("/", "+");

/**
 * Initialize a pager for theme('pager') without running an SQL query.
 *
 * The nuts and bolts of this function are borrowed from z3950.module
 * and also apachesolr.module.
 *
 * @see pager_query()
 *
 * @param $total
 *  The total number of items found.
 * @param $limit
 *  The number of items you will display per page.
 * @param $element
 *  An optional integer to distinguish between multiple pagers on one page.
 *
 * @return
 *  The current page for $element. 0 by default if $_GET['page'] is empty.
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

  // return the current position
  return $pager_page_array[$element];
}

/**
 * Restore slashes in a string.
 *
 * @param type $str
 * @return type
 */
function restore_slashes($str) {
  $replacements = array("~slsh~", "~plus~");
  $values = array("/", "+");
  return str_replace($replacements, $values, $str); //can't have a slash in the url breaks drupal clean urls
}

/**
 * Replace slashes in a string.
 *
 * @param type $str
 * @return type
 */
function replace_slashes($str) {
  $replacements = array("~slsh~", "~plus~");
  $values = array("/", "+");
  return str_replace($values, $replacements, $str); // reverse of above
}

/**
 * A better explode method allows quotes in the returned strings. Taken from
 * php.net.
 *
 * @param type $delim
 * @param type $str
 * @param type $enclose
 * @param type $preserve
 * @return type
 */
function csv_explode($delim = ',', $str, $enclose = '"', $preserve = FALSE) {
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
 * Copied from Apache Solr Service and added space as a value to escape.  We are
 * using this to escape the field being searched not the text being searched for
 * so we need both.
 *
 * @param type $value
 * @return type
 */
function lesser_escape($value) {
  //list taken from http://lucene.apache.org/java/docs/queryparsersyntax.html#Escaping%20Special%20Characters
  // Zac removed the quotation mark and whitespace from this list.
  // a) Spaces in our search strings don't need escaping
  // b) We want to allow users to use quotation marks to indicate multi-word phrases
  //  (alan) pulled the asterisk from excaping to allow wild cards
  $pattern = '/(\+|-|&&|\|\||!|\(|\)|\{|}|\[|]|\^|~|\?|:|\\\)/';
  $replace = '\\\$1';

  return preg_replace($pattern, $replace, $value);
}

//escape characters in field names of facets
function solr_escape($facets) {
  $return_facets = array();
  foreach ($facets as $facet) {
    $tmp = drupal_substr($facet, 0, strpos($facet, ':"'));
    $tmp = lesser_escape(trim($tmp));

    $return_facets[] = $tmp . drupal_substr($facet, strpos($facet, ':"'), drupal_strlen($facet));
  }
  return $return_facets;
}

/**
 * Turns term fields strings from the admin forms into arrays. Mostly used with
 * variable_get().
 *
 * @param type $raw_field_list
 * @return type
 */
function islandora_build_substitution_list($raw_field_list) {
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
      $rows[$field]['class'] = drupal_strtolower( preg_replace('/[^A-Za-z0-9]/', '-', $field));
      $rows[$field]['value'] = $value;
    }
    // Replace Solr doc.
    $object_results[$object_index]['solr_doc'] = $rows;
  }
  return $object_results;
}
