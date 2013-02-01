<?php

/**
 * @file
 *
 * Includes functions for advanced facet functionality.
 */

/**
 * Implements hook_forms().
 */
function islandora_solr_forms($form_id, $args) {

  // Check if the form_id passed to drupal_get_form() contains the string 'islandora_solr_search_date_filter_form'
  if (strpos($form_id, 'islandora_solr_date_filter_form') !== FALSE) {
    $forms[$form_id] = array(
      'callback' => 'islandora_solr_date_filter_form',
      'callback arguments' => array($args[0]),
    );
    return $forms;
  }

  // Check if the form_id passed to drupal_get_form() contains the string 'islandora_solr_range_slider_form'
  if (strpos($form_id, 'islandora_solr_range_slider_form') !== FALSE) {
    $forms[$form_id] = array(
      'callback' => 'islandora_solr_range_slider_form',
      'callback arguments' => array($args[0]),
    );
    return $forms;
  }
}

/**
 * Date filter form
 */
function islandora_solr_date_filter_form($form, &$form_state, $elements) {
  global $_islandora_solr_queryclass;
  extract($elements);
  
  $form = array(
    '#tree' => TRUE,
    '#prefix' => '<div class="islandora-solr-date-filter">',
    '#suffix' => '</div>',
  );
  // Field
  $form['date_filter_facet_field'] = array(
    '#type' => 'hidden',
    '#value' => $facet_field,
    '#name' => 'date_filter_facet_field_' . $form_key,
  );

  // check if default value is possible
  // parameters set in url
  $params = isset($_islandora_solr_queryclass->internalSolrParams) ? $_islandora_solr_queryclass->internalSolrParams : array();
  $filter_count = 0;
  if (isset($params['f'])) {
    $format = 'Y/m/d';
    foreach ($params['f'] as $key => $filter) {
      if (strpos($filter, $facet_field) === 0) {
        $filter_count++;
        // split the filter into field and value
        $filter_split = explode(':', $filter, 2);
        // trim brackets
        $filter_split[1] = trim($filter_split[1], "\"");
        // split range filter string to return formatted date values
        $filter_str = $filter_split[1];

        $filter_str = trim($filter_str, '[');
        $filter_str = trim($filter_str, ']');
        $filter_array = explode(' TO ', $filter_str);

        // get timestamps
        $from_unix = strtotime(trim($filter_array[0]));
        $to_unix = strtotime(trim($filter_array[1]));

        // only set default times if from date is lower than to date
        if ($from_unix < $to_unix) {
          $from_default = (strpos($filter_array[0], '*') !== FALSE) ? '*' : format_date($from_unix, 'custom', $format, 'UTC');
          $to_default = (strpos($filter_array[1], '*') !== FALSE) ? '*' : format_date($to_unix, 'custom', $format, 'UTC');
        }
        else {
          $from_default = NULL;
          $to_default = NULL;
        }
      }
    }
  }
  if ($filter_count != 1) {
    $from_default = NULL;
    $to_default = NULL;
  }

  if ($from_default != NULL || $to_default != NULL) {
    $class = 'date-range-expanded';
    $value = t('Hide');
  }
  else {
    $class = 'date-range-collapsed';
    $value = t('Show');
  }

  $form['date_range_expand'] = array(
    '#markup' => t('Specify date range: <a href="#" class="toggle-date-range-filter @class">@value</a>', array('@class' => $class, '@value' => $value)),
    '#prefix' => '<span class="date-filter-toggle-text">',
    '#suffix' => '</span>',
  );
  // Show/hide link
  $form['date_filter'] = array(
    '#prefix' => '<div class="date-range-filter-wrapper">',
    '#suffix' => '</div>',
  );
  // description
  $form['date_filter']['date_filter'] = array(
    '#markup' => '<div class="description">' . t('Format: @date', array('@date' => date("Y/m/d"))) . '</div>',
  );
  // From
  $form['date_filter']['date_filter_from'] = array(
    '#type' => 'textfield',
    '#title' => t('From'),
    '#default_value' => ($from_default) ? $from_default : '',
    '#size' => 10,
    '#maxsize' => 10,
    '#attributes' => array('class' => array('islandora-solr-datepicker-' . $form_key)),
  );
  // To
  $form['date_filter']['date_filter_to'] = array(
    '#type' => 'textfield',
    '#title' => t('To'),
    '#default_value' => ($to_default) ? $to_default : '',
    '#size' => 10,
    '#maxsize' => 10,
    '#attributes' => array('class' => array('islandora-solr-datepicker-' . $form_key)),
  );
  // Submit
  $form['date_filter']['date_filter_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#name' => 'date_filter_facet_field_' . $form_key,
    '#form_key' => $form_key,
  );
  $form['#submit'][] = 'islandora_solr_date_filter_form_submit';
  // Validate
  $form['#validate'][] = 'islandora_solr_date_filter_form_validate';
    
  // include datepicker js
  drupal_add_library('system', 'ui.datepicker');
  // add datepicker css
  drupal_add_css('misc/ui/jquery.ui.datepicker.css');
  // add js settings
  drupal_add_js(array(
    'islandoraSolrDatepickerRange' => array(
      $facet_field => array(
        'datepickerRange' => trim($datepicker_range),
        'formKey' => $form_key,
    ))), 'setting');

  return $form;
}

/**
 * Date filter form validate
 */
function islandora_solr_date_filter_form_validate($form, &$form_state) {
  $from = explode('/', $form_state['values']['date_filter']['date_filter_from']);
  $to = explode('/', $form_state['values']['date_filter']['date_filter_to']);
  $form_key = $form_state['triggering_element']['#form_key'];
  // default for month and day
  $default = '01';

  // if the 'from' value is '*' just skip all checks
  if (trim($from[0]) != '*' ) {
    // Apply some defaults
    if (!isset($from[1])) {
      $from[1] = $default;
    }
    if (!isset($from[2])) {
      $from[2] = $default;
    }

    // check from date
    if (!checkdate(intval($from[1]), intval($from[2]), intval($from[0]))) {
      form_set_error($form_key . '][date_filter_from', t('From date isn\'t formatted correctly.'));
    }
  }
  // if the 'to' value is '*' just skip all checks
  if (trim($to[0]) != '*' ) {
    // Apply some defaults
    if (!isset($to[1])) {
      $to[1] = $default;
    }
    if (!isset($to[2])) {
      $to[2] = $default;
    }
    // check to date
    if (!checkdate(intval($to[1]), intval($to[2]), intval($to[0]))) {
      form_set_error($form_key . '][date_filter_to', t('To date isn\'t formatted correctly.'));
    }
  }
}

/**
 * Date filter form submit
 */
function islandora_solr_date_filter_form_submit($form, &$form_state) {
  global $_islandora_solr_queryclass;
  $form_state['rebuild'] = TRUE;
  $params = isset($_islandora_solr_queryclass->internalSolrParams) ? $_islandora_solr_queryclass->internalSolrParams : array();
  $path = SOLR_SEARCH_PATH . '/' . replace_slashes($_islandora_solr_queryclass->solrQuery);
  $facet_field = $form_state['values']['date_filter_facet_field'];
  $form_key = $form_state['triggering_element']['#form_key'];

  // date
  $from = explode('/', $form_state['values']['date_filter']['date_filter_from']);
  $to = explode('/', $form_state['values']['date_filter']['date_filter_to']);

  // if the 'from' value is '*' just skip all checks
  if (trim($from[0]) != '*' ) {
    // Apply some defaults
    $default = '01';
    if (!isset($from[1])) {
      $from[1] = $default;
    }
    if (!isset($from[2])) {
      $from[2] = $default;
    }
    // create date string
    // example:
    // timestamp:[1900-01-01T00:00:00Z TO 1990-01-01T00:00:00Z]
    $from_str = "{$from[0]}-{$from[1]}-{$from[2]}T00:00:00Z";
  }
  else {
    $from_str = $from[0];
  }

  // if the 'to' value is '*' just skip all checks
  if (trim($to[0]) != '*' ) {
    // Apply some defaults
    if (!isset($to[1])) {
      $to[1] = $default;
    }
    if (!isset($to[2])) {
      $to[2] = $default;
    }
    // create date string
    // example:
    // timestamp:[1900-01-01T00:00:00Z TO 1990-01-01T00:00:00Z]
    $to_str = "{$to[0]}-{$to[1]}-{$to[2]}T00:00:00Z";
  }
  else {
    $to_str = $to[0];
  }
  // create filter
  $filter = "{$facet_field}:[{$from_str} TO {$to_str}]";

  // set date filter key if there are no date filters included
  if (isset($params['f'])) {
    foreach ($params['f'] as $key => $f) {
      if (strpos($f, $facet_field) !== FALSE) {
        array_splice($params['f'], $key);
      }
    }
    $params['f'][] = $filter;
    $query = $params;
  }
  else {
    $query = array_merge_recursive($params, array('f' => array($filter)));
  }

  // redirect
  drupal_goto($path, array('query' => $query));
}

/**
 * Range slider form.
 */
function islandora_solr_range_slider_form($form, &$form_state, $elements) {
  global $_islandora_solr_queryclass;
  extract($elements);
  $from_default = current($data);
  $to_default = end($data);
  if (!empty($gap)) {
    $gap = "({$gap})";
  }

  $form = array(
    '#tree' => TRUE,
    '#prefix' => '<div class="islandora-solr-range-slider">',
    '#suffix' => '</div>',
  );
  // Field
  $form['range_slider_term'] = array(
    '#type' => 'hidden',
    '#value' => $facet_field,
    '#name' => 'range_slider_term_' . $form_key,
  );
  $slider_variables = array(
    'form_key' => $form_key,
    'gap' => $gap,
    'range_from' => format_date(strtotime(trim($from_default['date'])) + 1, 'custom', $date_format, 'UTC'),
    'range_to' => format_date(strtotime(trim($to_default['date'])), 'custom', $date_format, 'UTC'),
  );  
  $form['markup'] = array(
    '#markup' => theme('islandora_solr_range_slider', $slider_variables),
  );
  // hidden from
  $form['range_slider_hidden_from'] = array(
    '#type' => 'hidden',
    '#default_value' => $from_default['date'], // lowest value
    '#attributes' => array('class' => array('range-slider-hidden-from-' . $form_key)),
  );
  // hidden to
  $form['range_slider_hidden_to'] = array(
    '#type' => 'hidden',
    '#default_value' => $to_default['date'], // highest value
    '#attributes' => array('class' => array('range-slider-hidden-to-' . $form_key)),
  );
  // Submit
  $form['range_slider_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#name' => 'range_slider_submit_' . $form_key,
    '#form_key' => $form_key,
  );
  $form['#submit'][] = 'islandora_solr_range_slider_form_submit';

  // include jquery ui slider
  drupal_add_library('system', 'ui.slider');
  // include flot
  // @TODO: use the libraries module for this!
  // @TODO: use the new version of flot. Didn't work out of the box, so needs some extra attention.
  drupal_add_js(drupal_get_path('module', 'islandora_solr') . '/js/flot/jquery.flot.min.js');  
  // @TODO: find a good way to call this for IE8 and lower
  // <!--[if lte IE 8]><![endif]-->
//  drupal_add_js(drupal_get_path('module', 'islandora_solr') . '/js/flot/excanvas.min.js');

  // include jquery ui css
  drupal_add_css('misc/ui/jquery.ui.slider.css');
  // include slider settings
  $js_settings = array(
    'islandoraSolrRangeSlider' => array(
      $facet_field => array(
        'facet_field' => $facet_field,
        'form_key' => $form_key,
        'data' => $data,
        'slider_color' => $slider_color,
      ),
    ),
  );
  drupal_add_js($js_settings, 'setting');

  drupal_add_js(drupal_get_path('module', 'islandora_solr') . '/js/islandora_solr_facets.js');

  return $form;
}

/**
 * Range slider form submit.
 */
function islandora_solr_range_slider_form_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;

  // set variables
  global $_islandora_solr_queryclass;
  $params = isset($_islandora_solr_queryclass->internalSolrParams) ? $_islandora_solr_queryclass->internalSolrParams : array();
  // set path
  $path = SOLR_SEARCH_PATH . '/' . replace_slashes($_islandora_solr_queryclass->solrQuery);
  // form key
  $form_key = $form_state['triggering_element']['#form_key'];
  // term
  $term = $form_state['values']['range_slider_term'];

  // date
  $from = $form_state['values']['range_slider_hidden_from'];
  $to = $form_state['values']['range_slider_hidden_to'];

  // create filter
  $filter = "{$term}:[{$from} TO {$to}]";

  // set date filter key if there are no date filters included
  if (isset($params['f'])) {
    foreach ($params['f'] as $key => $f) {
      if (strpos($f, $term) !== FALSE) {
        array_splice($params['f'], $key);
      }
    }
    $params['f'][] = $filter;
    $query = $params;
  }
  else {
    $query = array_merge_recursive($params, array('f' => array($filter)));
  }

  // redirect
  drupal_goto($path, array('query' => $query));
}

