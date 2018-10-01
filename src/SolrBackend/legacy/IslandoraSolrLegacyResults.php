<?php

/**
 * @file
 * Contains methods to create rendered Solr displays from raw Solr results.
 * Depends on Apache_Solr_Php client.
 */

namespace Drupal\islandora_solr\SolrBackend\legacy;

use Drupal\islandora_solr\SolrBackend\IslandoraSolrResultsInterface;

/**
 * Islandora Solr Results
 */
class IslandoraSolrLegacyResults implements IslandoraSolrResultsInterface {

  public $facetFieldArray = array();
  public $searchFieldArray = array();
  public $resultFieldArray = array();
  public $allSubsArray = array();
  public $islandoraSolrQueryProcessor;
  public $rangeFacets = array();
  public $dateFormatFacets = array();

  /**
   * Constructor.
   */
  public function __construct() {
    $this->prepFieldSubstitutions();
    $this->rangeFacets = islandora_solr_get_range_facets();
    $this->dateFormatFacets = islandora_solr_get_date_format_facets();
  }

  /**
   * {@inheritdoc}
   *
   * @throws \Exception
   *   If theme() is called before all modules are loaded, we do not necessarily
   *   have a full theme registry to work with, and therefore cannot process
   *   the theme request properly. See also _theme_load_registry().
   */
  public function displayResults($islandora_solr_query) {
    $this->islandoraSolrQueryProcessor = $islandora_solr_query;

    // Set variables to collect returned data.
    $results = NULL;
    $secondary_profiles = NULL;
    $elements = array();

    // Set breadcrumbs.
    $this->setBreadcrumbs($islandora_solr_query);

    // Raw solr results.
    $islandora_solr_result = $this->islandoraSolrQueryProcessor->islandoraSolrResult;

    // Solr results count.
    // Total Solr results.
    $elements['solr_total'] = (int) $islandora_solr_result['response']['numFound'];

    // Solr start.
    // To display: $islandora_solr_query->solrStart + ($total > 0 ? 1 : 0).
    $elements['solr_start'] = $islandora_solr_query->solrStart;

    // Solr results end.
    $end = min(($islandora_solr_query->solrLimit + $elements['solr_start']), $elements['solr_total']);
    $elements['solr_end'] = $end;

    // Pager.
    islandora_solr_pager_init($elements['solr_total'], $islandora_solr_query->solrLimit);
    $elements['solr_pager'] = theme('pager', array(
      'tags' => NULL,
      'element' => 0,
      'parameters' => NULL,
      'quantity' => 5,
    ));

    // Debug (will be removed).
    $elements['solr_debug'] = '';
    if (variable_get('islandora_solr_debug_mode', 0)) {
      $elements['solr_debug'] = $this->printDebugOutput($islandora_solr_result);
    }

    // Rendered secondary display profiles.
    $secondary_profiles = $this->addSecondaries($islandora_solr_query);

    // Rendered results.
    $results = $this->printResults($islandora_solr_result);

    return theme('islandora_solr_wrapper', array(
      'results' => $results,
      'secondary_profiles' => $secondary_profiles,
      'elements' => $elements,
    ));
  }

  /**
   * {@inheritdoc}
   *
   * @throws \Exception
   *   If theme() is called before all modules are loaded, we do not necessarily
   *   have a full theme registry to work with, and therefore cannot process
   *   the theme request properly. See also _theme_load_registry().
   */
  public function addSecondaries($islandora_solr_query) {
    $query_list = array();
    // Get secondary display profiles.
    $secondary_display_profiles = module_invoke_all('islandora_solr_secondary_display');

    // $_GET['q'] didn't seem to work here.
    $path = current_path();

    // Parameters set in URL.
    $params = $islandora_solr_query->internalSolrParams;

    // Get list of secondary displays.
    $secondary_array = variable_get('islandora_solr_secondary_display', array());
    foreach ($secondary_array as $name => $status) {
      if ($status === $name) {
        // Generate URL.
        $query_secondary = array_merge($params, array('solr_profile' => $name));

        // Set attributes variable for remove link.
        $attr = array();
        $attr['title'] = $secondary_display_profiles[$name]['description'];
        $attr['rel'] = 'nofollow';
        $attr['href'] = url($path, array('query' => $query_secondary));
        $logo = $secondary_display_profiles[$name]['logo'];

        // XXX: We are not using l() because of active classes:
        // @see http://drupal.org/node/41595
        // Create link.
        $query_list[] = '<a' . drupal_attributes($attr) . '>' . $logo . '</a>';
      }
    }

    return theme('item_list', array(
      'items' => $query_list,
      'title' => NULL,
      'type' => 'ul',
      'attributes' => array('id' => 'secondary-display-profiles'),
    ));
  }

  /**
   * {@inheritdoc}
   *
   * @throws \Exception
   *   If theme() is called before all modules are loaded, we do not necessarily
   *   have a full theme registry to work with, and therefore cannot process
   *   the theme request properly. See also _theme_load_registry().
   *
   */
  public function printResults($solr_results) {
    $solr_results = islandora_solr_prepare_solr_results($solr_results);
    $object_results = $solr_results['response']['objects'];
    $object_results = islandora_solr_prepare_solr_doc($object_results);

    $elements = array();
    $elements['solr_total'] = $solr_results['response']['numFound'];
    $elements['solr_start'] = $solr_results['response']['start'];

    // Return themed search results.
    return theme('islandora_solr', array('results' => $object_results, 'elements' => $elements));
  }


  /**
   * {@inheritdoc}
   *
   * @throws \Exception
   *   If theme() is called before all modules are loaded, we do not necessarily
   *   have a full theme registry to work with, and therefore cannot process
   *   the theme request properly. See also _theme_load_registry().
   */
  public function currentQuery($islandora_solr_query) {
    module_load_include('inc', 'islandora', 'includes/utilities');

    $output = '';
    $path = current_path();
    $format = variable_get('islandora_solr_facet_date_format', 'Y');

    // Get user provided filter parameters.
    $fq = isset($islandora_solr_query->internalSolrParams['f']) ? $islandora_solr_query->internalSolrParams['f'] : array();
    // Parameters set in URL.
    $params = $islandora_solr_query->internalSolrParams;
    // Get query values.
    if (!in_array($islandora_solr_query->solrQuery, $islandora_solr_query->differentKindsOfNothing)) {
      // Get query value.
      $query_value = stripslashes($islandora_solr_query->solrQuery);

      $query_list = array();
      if (variable_get('islandora_solr_human_friendly_query_block', TRUE)) {
        foreach ($this->searchFieldArray as $search_field => $search_field_label) {
          $query_value = str_replace($search_field . ':(', $search_field_label . ':(', $query_value);
        }
      }

      // Remove link keeps all parameters (query gets removed instead).
      $query_minus = $params;

      // Remove query from path.
      $path_minus = implode('/', explode('/', $path, -1));

      // Set attributes variable for remove link.
      $attributes = array(
        'minus' => array(
          'attr' => array(),
          'path' => $path_minus,
          'query' => $query_minus,
        ),
      );
      $attr_minus =& $attributes['minus']['attr'];
      $attr_minus['title'] = t('Remove') . ' ' . $query_value;
      $attr_minus['class'] = array('remove-query');
      $attr_minus['rel'] = 'nofollow';
      $attr_minus['href'] = url($path_minus, array('query' => $query_minus));

      $hooks = islandora_build_hook_list(ISLANDORA_SOLR_FACET_BUCKET_CLASSES_HOOK_BASE);
      drupal_alter($hooks, $attributes, $islandora_solr_query);

      // XXX: We are not using l() because of active classes:
      // @see http://drupal.org/node/41595
      // Create link.
      $query_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> ' . check_plain($query_value);

      // Add wrap and list.
      $output .= '<div class="islandora-solr-query-wrap">';
      $output .= theme('item_list', array(
        'items' => $query_list,
        'title' => t('Query'),
        'type' => 'ul',
        'attributes' => array('class' => 'islandora-solr-query-list query-list'),
      ));
      $output .= '</div>';

    }

    // Get filter values.
    if (!empty($fq)) {
      // Set list variables.
      $filter_list = array();
      foreach ($fq as $key => $filter) {
        // Check for exclude filter.
        if ($filter[0] == '-') {
          // Not equal sign.
          $symbol = '&ne;';
        }
        else {
          $symbol = '=';
        }
        $filter_string = $this->formatFilter($filter, $islandora_solr_query);
        // Pull out filter (for exclude link).
        $query_minus = array();
        $f_x['f'] = array_diff($params['f'], array($filter));
        $query_minus = array_merge($params, $f_x);
        // @todo Find a cleaner way to do this.
        // Resetting the filter keys' order.
        if ($query_minus['f']) {
          $query_minus['f'] = array_merge(array(), $query_minus['f']);
        }
        // Remove 'f' if empty.
        if (empty($query_minus['f'])) {
          unset($query_minus['f']);
        }
        // Set attributes variable for remove link.
        $attributes = array(
          'minus' => array(
            'attr' => array(),
            'path' => $path,
            'query' => $query_minus,
          ),
        );
        $attr_minus =& $attributes['minus']['attr'];
        $attr_minus['title'] = t('Remove') . ' ' . $filter;
        $attr_minus['class'] = array('remove-filter');
        $attr_minus['rel'] = 'nofollow';
        $attr_minus['href'] = url($path, array('query' => $query_minus));

        $hooks = islandora_build_hook_list(ISLANDORA_SOLR_FACET_BUCKET_CLASSES_HOOK_BASE);
        drupal_alter($hooks, $attributes, $islandora_solr_query);

        // XXX: We are not using l() because of active classes:
        // @see http://drupal.org/node/41595
        // Create link.
        $filter_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> ' . $symbol . ' ' . check_plain($filter_string);

      }

      // Return filter list.
      $output .= '<div class="islandora-solr-filter-wrap">';
      $output .= theme('item_list', array(
        'items' => $filter_list,
        'title' => t("Enabled Filters"),
        'type' => 'ul',
        'attributes' => array('class' => 'islandora-solr-filter-list filter-list'),
      ));
      $output .= '</div>';
    }
    return $output;
  }

  /**
   * {@inheritdoc}
   */
  public function setBreadcrumbs($islandora_solr_query) {
    $breadcrumb = $this->getBreadcrumbs($islandora_solr_query);
    drupal_set_breadcrumb($breadcrumb);
  }

  /**
   * {@inheritdoc}
   */
  public function getBreadcrumbs($islandora_solr_query) {
    // $_GET['q'] didn't seem to work here.
    $path = current_path();
    // Get date format.
    $format = variable_get('islandora_solr_facet_date_format', 'Y');

    $breadcrumb = array();
    // Get user provided filter parameters.
    if (isset($islandora_solr_query->internalSolrParams['f'])) {
      $fq = $islandora_solr_query->internalSolrParams['f'];
    }
    else {
      $fq = array();
    }

    // Parameters set in URL.
    $params = $islandora_solr_query->internalSolrParams;
    // Set filter key if there are no filters included.
    if (empty($params['f'])) {
      $params['f'] = array();
    }

    // Loop to create filter breadcrumbs if available.
    if (!empty($fq)) {
      $f['f'] = array();
      foreach ($fq as $key => $filter) {
        // Check for exclude filter.
        $exclude = FALSE;
        if ($filter[0] == '-') {
          $exclude = TRUE;
        }
        $filter_string = $this->formatFilter($filter, $islandora_solr_query);
        // Increment filter array with current filter (for breadcrumb link).
        $query = array();
        $query_diff = $params;
        if (isset($query_diff['f'])) {
          unset($query_diff['f']);
        }
        $f = array_merge_recursive($f, array('f' => array($filter)));
        $query = array_merge($query_diff, $f);

        // Pull out filter (for x link).
        $query_x = array();
        $f_x['f'] = array_diff($params['f'], array($filter));
        $query_x = array_merge($params, $f_x);
        // @todo Find a cleaner way to do this.
        // Resetting the filter key's order.
        if ($query_x['f']) {
          $query_x['f'] = array_merge(array(), $query_x['f']);
        }
        // Remove 'f' if empty.
        if (empty($query_x['f'])) {
          unset($query_x['f']);
        }

        // Set attributes variable.
        $attr = array();
        $attr['title'] = $filter;
        $attr['rel'] = 'nofollow';
        if ($exclude) {
          $attr['class'] = 'strikethrough';
        }
        $attr['href'] = url($path, array('query' => $query));

        // Set attributes variable for remove link.
        $attr_x = array();
        $attr_x['title'] = t('Remove') . ' ' . $filter;
        $attr_x['rel'] = 'nofollow';
        $attr_x['href'] = url($path, array('query' => $query_x));

        // XXX: We are not using l() because of active classes:
        // @see http://drupal.org/node/41595
        // Create link.
        $breadcrumb[] = '<a' . drupal_attributes($attr) . '>' . check_plain($filter_string) . '</a>'
              . '<span class="islandora-solr-breadcrumb-super"> <a' . drupal_attributes($attr_x) . '>(' . t('x') . ')</a></span>';

      }
      // At this point reverse the breadcrumbs array (only contains filters).
      $breadcrumb = array_reverse($breadcrumb);
    }

    // Create query breadcrumb.
    if (!in_array($islandora_solr_query->solrQuery, $islandora_solr_query->differentKindsOfNothing)) {
      // Get query value.
      $query_value = $islandora_solr_query->solrQuery;
      // Remove all filters for this breadcrumb.
      $query = $params;
      if (isset($query['f'])) {
        unset($query['f']);
      }
      // Remove button keeps all parameters (query gets removed instead).
      $query_x = array();
      $query_x = $params;
      if (empty($params['f'])) {
        unset($query_x['f']);
      }
      // Remove query from path.
      $path_x = implode('/', explode('/', $path, -1)) . '/ ';

      // Set attributes variable.
      $attr = array();
      $attr['title'] = $query_value;
      $attr['rel'] = 'nofollow';
      $attr['href'] = url($path, array('query' => $query));

      // Set attributes variable for remove link.
      $attr_x = array();
      $attr_x['title'] = t('Remove') . ' ' . $query_value;
      $attr_x['rel'] = 'nofollow';
      $attr_x['href'] = url($path_x, array('query' => $query_x));

      // Remove solr fields from breadcrumb value.
      $query_explode = preg_split(ISLANDORA_SOLR_QUERY_SPLIT_REGEX, $query_value);
      $query_implode = array();
      foreach ($query_explode as $value) {
        // Check for first colon to split the string.
        if (strpos($value, ':') != FALSE) {
          // Split the filter into field and value.
          $value_split = preg_split(ISLANDORA_SOLR_QUERY_FIELD_VALUE_SPLIT_REGEX, $value, 2);
          // Trim whitespace.
          $value_split[1] = trim($value_split[1]);
          // Trim brackets.
          $value = str_replace(array('(', ')'), '', $value_split[1]);
        }
        // No colon is found.
        else {
          $value = trim($value);
          // Strip brackets.
          $value = str_replace(array('(', ')'), '', $value);
        }
        $query_implode[] = $value;
      }
      $query_value = implode(" ", $query_implode);

      // XXX: We are not using l() because of active classes:
      // @see http://drupal.org/node/41595
      // Create link.
      $breadcrumb[] = '<a' . drupal_attributes($attr) . '>' . stripslashes(check_plain($query_value)) . '</a>'
            . '<span class="islandora-solr-breadcrumb-super"> <a' . drupal_attributes($attr_x) . '>(' . t('x') . ')</a></span>';
    }

    $breadcrumb[] = l(t('Home'), '<front>', array('attributes' => array('title' => t('Home'))));
    if (!empty($breadcrumb)) {
      $breadcrumb = array_reverse($breadcrumb);
    }
    $context = 'solr';
    drupal_alter('islandora_breadcrumbs', $breadcrumb, $context);
    return $breadcrumb;
  }

  /**
   * {@inheritdoc}
   */
  public function formatFilter($filter, $islandora_solr_query) {
    // @todo See how this interacts with multiple date filters.
    // Check if there are operators in the filter.
    $fq_split = preg_split('/ (OR|AND) /', $filter);
    if (count($fq_split) > 1) {
      $operator_split = preg_split(ISLANDORA_SOLR_QUERY_SPLIT_REGEX, $filter);
      $operator_split = array_diff($operator_split, $fq_split);
      $out_array = array();
      foreach ($fq_split as $fil) {
        $fil_split = preg_split(ISLANDORA_SOLR_QUERY_FIELD_VALUE_SPLIT_REGEX, $fil, 2);
        $out_str = str_replace(array('"', 'info:fedora/'), '', $fil_split[1]);
        $out_array[] = $out_str;
      }
      $filter_string = '';
      foreach ($out_array as $out) {
        $filter_string .= $out;
        if (count($operator_split)) {
          $filter_string .= ' ' . array_shift($operator_split) . ' ';
        }
      }
      $filter_string = trim($filter_string);
    }
    else {
      // Split the filter into field and value.
      $filter_split = preg_split(ISLANDORA_SOLR_QUERY_FIELD_VALUE_SPLIT_REGEX, $filter, 2);
      // Trim brackets.
      $filter_split[1] = trim($filter_split[1], "\"");
      $solr_field = ltrim($filter_split[0], '-');
      // If value is date.
      if (isset($islandora_solr_query->solrParams['facet.date']) && in_array($solr_field, $islandora_solr_query->solrParams['facet.date'])) {
        // Check date format setting.
        foreach ($this->rangeFacets as $value) {
          if ($value['solr_field'] == $solr_field && isset($value['solr_field_settings']['date_facet_format']) && !empty($value['solr_field_settings']['date_facet_format'])) {
            $format = $value['solr_field_settings']['date_facet_format'];
          }
        }
        // Split range filter string to return formatted date values.
        $filter_str = $filter_split[1];
        $filter_str = trim($filter_str, '[');
        $filter_str = trim($filter_str, ']');
        $filter_array = explode(' TO ', $filter_str);
        $filter_split[1] = format_date(strtotime(trim($filter_array[0])) + (60 * 60 * 24), 'custom', $format) . ' - ' . format_date(strtotime(trim($filter_array[1])) + (60 * 60 * 24), 'custom', $format);
      }
      elseif (isset($this->dateFormatFacets[$solr_field])) {
        $format = $this->dateFormatFacets[$solr_field]['solr_field_settings']['date_facet_format'];
        $filter_split[1] = format_date(strtotime(stripslashes($filter_split[1])), 'custom', $format);
      }
      $filter_string = $filter_split[1];
    }
    return stripslashes($filter_string);
  }

  /**
   * {@inheritdoc}
   */
  public function displayFacets($islandora_solr_query) {
    IslandoraSolrLegacyFacets::init($islandora_solr_query);
    $output = '';
    $facet_order = $this->facetFieldArray;
    foreach ($facet_order as $facet_key => $facet_label) {
      $facet_obj = new IslandoraSolrFacets($facet_key);
      $output .= $facet_obj->getFacet();
    }

    // As we add additional facets, we're repeatedly URL-encoding old facet
    // strings. when we double-encode quotation marks they're incomprehensible
    // to Solr.
    $output = str_replace('%2B', '%252B', $output);
    return $output;
  }

  /**
   * Create a fieldset for debugging purposes.
   *
   * Creates a fieldset containing raw Solr results of the current page for
   * debugging purposes.
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $islandora_solr_results
   *   The processed Solr results from
   *   IslandoraSolrQueryProcessor::islandoraSolrResult
   *
   * @return string
   *   Rendered fieldset containing raw Solr results data.
   * @throws \Exception
   *   If called before all modules are loaded, we do not necessarily have a full
   *   theme registry to work with, and therefore cannot process the theme
   *   request properly. See also _theme_load_registry().
   */
  public function printDebugOutput($islandora_solr_results) {
    // Debug dump.
    $results = "<pre>Results: " . print_r($islandora_solr_results, TRUE) . "</pre>";
    $fieldset = array(
      '#title' => t("Islandora Processed Solr Results"),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#value' => $results,
      '#children' => '',
    );

    return theme('fieldset', array('element' => $fieldset));
  }

  /**
   * Reads configuration values and prepares field => label mappings.
   *
   * Reads configuration values and preps a number of key => value arrays for
   * output substitution. Replaces solr field labels with human readable labels
   * as set in the admin form.
   */
  public function prepFieldSubstitutions() {

    $this->facetFieldArray = islandora_solr_get_fields('facet_fields');

    $this->searchFieldArray = islandora_solr_get_fields('search_fields');

    $this->resultFieldArray = islandora_solr_get_fields('result_fields');

    $this->allSubsArray = array_merge($this->facetFieldArray, $this->searchFieldArray, $this->resultFieldArray);
  }
}
