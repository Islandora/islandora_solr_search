<?php

/**
 * @file
 * Theme functions for the Islandora Solr Search module.
 */


/**
 * Prepares variables for islandora_solr_wrapper templates.
 *
 * Default template: theme/islandora-solr-wrapper.tpl.php.
 *
 * @global string $base_url
 *
 * @param array $variables
 *   An associative array containing:
 *   - results: Rendered primary display
 *   - secondary_profiles: (optional) Rendered list of secondary display
 *     profiles
 *   - elements: additional elements to populate the results page
 *     - pager: (optional) Rendered pager
 *     - solr_total: (optional) Total amount of documents found.
 *     - solr_start: (optional) Solr start position
 *     - solr_end: (optional) Solr end position
 *
 * @see islandora_solr_theme()
 * @see IslandoraSolrResults::displayResults()
 */
function islandora_solr_preprocess_islandora_solr_wrapper(&$variables) {
  global $base_url;
  $variables['base_url'] = $base_url;

  $elements = $variables['elements'];

  // Make all variables in 'elements' available as variables in the template
  // file.
  foreach ($variables['elements'] as $key => $value) {
    $variables[$key] = $value;
  }

  // Results count.
  $total = $elements['solr_total'];
  $end = $elements['solr_end'];
  $start = $elements['solr_start'] + ($total > 0 ? 1 : 0);

  // Format numbers.
  $total = number_format($total, 0, '.', ',');
  $end = number_format($end, 0, '.', ',');
  $start = number_format($start, 0, '.', ',');

  $variables['islandora_solr_result_count'] = t('(@start - @end of @total)', array(
    '@start' => $start,
    '@end' => $end,
    '@total' => $total,
  ));
}

/**
 * Prepares variables for islandora_solr templates.
 *
 * Default template: theme/islandora-solr.tpl.php.
 */
function islandora_solr_preprocess_islandora_solr(&$variables) {
  $results = $variables['results'];
  foreach ($results as $key => $result) {
    // Thumbnail.
    $path = url($result['thumbnail_url'], array('query' => $result['thumbnail_url_params']));
    $image = theme('image', array('path' => $path));

    $options = array('html' => TRUE);
    if (isset($result['object_label'])) {
      $options['attributes']['title'] = $result['object_label'];
    }
    if (isset($result['object_url_params'])) {
      $options['query'] = $result['object_url_params'];
    }
    if (isset($result['object_url_fragment'])) {
      $options['fragment'] = $result['object_url_fragment'];
    }
    // Thumbnail link.
    $variables['results'][$key]['thumbnail'] = l($image, $result['object_url'], $options);
  }
}

/**
 * Prepares variables for islandora_solr_facet templates.
 *
 * Default template: theme/islandora-solr-facet.tpl.php.
 */
function islandora_solr_preprocess_islandora_solr_facet(&$variables) {
  $buckets = $variables['buckets'];
  if ($variables['hidden']) {
    $variables['classes_array'][] = 'hidden';
  }
}

/**
 * Returns HTML for an islandora_solr_facet_wrapper.
 *
 * @param array $variables
 *   An associative array containing:
 *   - title: A string to use as the header/title.
 *   - content: A string containing the content being wrapped.
 *
 * @ingroup themeable
 */
function theme_islandora_solr_facet_wrapper($variables) {
  $output = '<div class="islandora-solr-facet-wrapper">';
  $output .= '<h3>' . $variables['title'] . '</h3>';
  $output .= $variables['content'];
  $output .= '</div>';
  return $output;
}
