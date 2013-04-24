<?php

/**
 * @file
 * This file contains DB query functions
 */

/**
 * Performs a select query to get fields for any field type.
 *
 * @params string $field_type
 *   The type of field to query for. If not passed, the query returns all fields.
 * @params bool $filter
 *   If set to TRUE, it will filter the array agains user roles. If set to FALSE, it will return all results.
 * @params bool $simplify
 *   Whether to return the fields array as a simple term => label array.
 * @return array
 *   The query results as an array.
 *
 * @see islandora_solr_get_fields()
 */
function islandora_solr_get_fields($field_type = NULL, $filter = TRUE, $simplified = TRUE, $keys = FALSE) {
  global $base_path;
  // fields query
  $query = db_select('islandora_solr_fields', 'isf');
  $query->fields('isf'); // all fields
  if ($field_type !== NULL) {
    $query->condition('isf.field_type', $field_type, '=');
  }
  $query->orderBy('isf.weight', 'ASC');
  try {
    $result = $query->execute();
    $records = $result->fetchAll(PDO::FETCH_ASSOC);
  }
  catch (Exception $e) {
    if ($e->errorInfo[0] == '42S02') {
      drupal_set_message(t('Islandora Solr fields table not found. Try running <a href="@update_url">update.php</a>.', array('@update_url' => url('update.php'))), 'error');
    }
    else {
      drupal_set_message($e->getMessage(), 'error');
    }
    $records = array();
  }
  // unserialize solr field settings blob
  foreach ($records as $key => $value) {
    $records[$key]['solr_field_settings'] = unserialize($records[$key]['solr_field_settings']);
  }
  // filter
  if ($filter == TRUE) {
    $records = _islandora_solr_filter_fields($records);
  }
  // simplify
  if ($simplified == TRUE) {
    $records = _islandora_solr_simplify_fields($records);
  }
  // fields as keys
  if ($keys == TRUE && $simplified == FALSE) {
    foreach ($records as $key => $value) {
      $records[$value['solr_field']] = $value;
      unset($records[$key]);
    }
  }
  return $records;
}

/**
 * Checks an array of fields DB records against user roles
 *
 * @param array $records
 *   An array of fields.
 * @return array
 *   The array of fields, filtered by user roles.
 *
 * @see islandora_solr_get_fields()
 */
function _islandora_solr_filter_fields($records = array()) {
  global $user;
  // no checks for user 1
  if ($user->uid == 1) {
    return $records;
  }
  $filtered_records = array();
  foreach ($records as $key => $record) {
    // if field settings are set, check permissions.
    if (isset($record['solr_field_settings']) && !empty($record['solr_field_settings'])) {
      // get field permissions
      $field_roles = $record['solr_field_settings']['permissions'];
      // check field permissions agains user roles
      $check = array_intersect($field_roles, array_keys($user->roles));
      if (!empty($check)) {
        $filtered_records[] = $record;
      }
    }
    else { // if permissions are never explicitly set, include by default
      $filtered_records[] = $record;
    }
  }
  return $filtered_records;
}

/**
 * Formats fields data from the DB as a clean array.
 *
 * @param array $fields
 *   Raw array from the DB with fields data.
 * @return array
 *   Returns a cleaned up array as term => label
 *
 * @see islandora_solr_get_fields()
 */
function _islandora_solr_simplify_fields($fields = array()) {
  // render as term => label
  $records = array();
  foreach ($fields as $key => $value) {
    $term = $value['solr_field'];
    if (isset($value['solr_field_settings']['label']) && !empty($value['solr_field_settings']['label'])) {
      $label = $value['solr_field_settings']['label'];
    }
    else {
      $label = $term;
    }
    $records[$term] = $label;
  }
  return $records;
}

/**
 * Return display fields with snippet highlighting enabled.
 */
function islandora_solr_get_snippet_fields() {
  $records = islandora_solr_get_fields('result_fields', TRUE, FALSE);
  $snippet_fields = array();
  foreach ($records as $key => $value) {
    if (isset($value['solr_field_settings']['snippet']) && $value['solr_field_settings']['snippet'] == 1) {
      $snippet_fields[] = $value['solr_field'];
    }
  }
  return $snippet_fields;
}

/**
 * Return display fields with 'link to object' enabled.
 */
function islandora_solr_get_link_to_object_fields() {
  $records = islandora_solr_get_fields('result_fields', TRUE, FALSE);
  $link_to_object_fields = array();
  foreach ($records as $key => $value) {
    if (isset($value['solr_field_settings']['link_to_object']) && $value['solr_field_settings']['link_to_object'] == 1) {
      $link_to_object_fields[] = $value['solr_field'];
    }
  }
  return $link_to_object_fields;
}

/**
 * Return display fields with 'date format' enabled.
 */
function islandora_solr_get_date_format_fields() {
  $records = islandora_solr_get_fields('result_fields', TRUE, FALSE);
  $date_format = array();
  foreach ($records as $key => $value) {
    if (isset($value['solr_field_settings']['date_format']) && !empty($value['solr_field_settings']['date_format'])) {
      $date_format[$value['solr_field']] = $value['solr_field_settings']['date_format'];
    }
  }
  return $date_format;
}

/**
 * Return facet fields with range enabled.
 */
function islandora_solr_get_range_facets() {
  $records = islandora_solr_get_fields('facet_fields', TRUE, FALSE);
  $range_facets = array();
  foreach ($records as $key => $value) {
    if (isset($value['solr_field_settings']['range_facet_select']) && $value['solr_field_settings']['range_facet_select'] == 1) {
      $range_facets[] = $value;
    }
  }
  return $range_facets;
}
