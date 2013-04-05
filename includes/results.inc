<?php

/**
 * @file
 *   Contains methods to create rendered Solr displays from raw Solr results.
 *   Depends on Apache_Solr_Php client.
 */

/**
 * Islandora Solr Results
 */
class IslandoraSolrResults {

  public $facetFieldArray = array();
  public $searchFieldArray = array();
  public $resultFieldArray = array();
  public $allSubsArray = array();
  public $islandoraSolrQueryProcessor;
  public $rangeFacets = array();

  /**
   * Constructor
   */
  function IslandoraSolrResults() {
    $this->prepFieldSubstitutions();
    $this->rangeFacets = islandora_solr_get_range_facets();
  }

  /**
   * Output the main body of the search results
   *
   * @see islandora_solr()
   *
   * @param IslandoraSolrQueryProcessor $islandoraSolrQuery
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return
   *   Returns themed Solr results page, including wrapper and rendered search
   *   results.
   */
  function displayResults($islandoraSolrQuery) {
    $this->islandoraSolrQueryProcessor = $islandoraSolrQuery;
    // set variables to collect returned data.
    $results = NULL;
    $secondary_profiles = NULL;
    $elements = array();

    // set breadcrumbs
    $this->setBreadcrumbs($islandoraSolrQuery);

    // raw solr results
    $islandora_solr_result = $this->islandoraSolrQueryProcessor->islandoraSolrResult;

    // solr results count
    // total solr results
    $elements['solr_total'] = (int) $islandora_solr_result['response']['numFound'];

    // solr start
    // to display: $islandoraSolrQuery->solrStart + ($total > 0 ? 1 : 0)
    $elements['solr_start'] = $islandoraSolrQuery->solrStart;

    // solr results end
    $end = min(($islandoraSolrQuery->solrLimit + $elements['solr_start']), $elements['solr_total']);
    $elements['solr_end'] = $end;

    // pager
    islandora_solr_pager_init($elements['solr_total'], $islandoraSolrQuery->solrLimit);
    $elements['solr_pager'] = theme('pager', array(NULL, $islandoraSolrQuery->solrLimit, 0, NULL, 5));

    // debug (will be removed)
    $elements['solr_debug'] = '';
    if (variable_get('islandora_solr_debug_mode', 0)) {
      $elements['solr_debug'] = $this->printDebugOutput($islandora_solr_result);
    }

    // rendered secondary display profiles
    $secondary_profiles = $this->addSecondaries($islandoraSolrQuery);

    // rendered results
    $results = $this->printResults($islandora_solr_result);

    // return themed layout
    return theme('islandora_solr_wrapper', array('results' => $results, 'secondary_profiles' => $secondary_profiles, 'elements' => $elements));
  }

  /**
   * Renders the secondary display profile list
   *
   * @param type $islandoraSolrQuery
   *   the IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   * @return
   *   Themed list of secondary displays
   *
   * @see IslandoraSolrResults::displayResults()
   */
  function addSecondaries($islandoraSolrQuery) {
    $query_list = array();
    // Get secondary display profiles
    $secondary_display_profiles = module_invoke_all('islandora_solr_secondary_display');

    // set path
    $path = SOLR_SEARCH_PATH . '/' . replace_slashes($islandoraSolrQuery->solrQuery); // $_GET['q'] didn't seem to work here

    // parameters set in url
    $params = $islandoraSolrQuery->internalSolrParams;

    // get list of secondary displays
    $secondary_array = variable_get('islandora_solr_secondary_display', array());

    foreach ($secondary_array as $name => $status) {
      if ($status === $name) {
        // generate url
        $query_secondary = array_merge($params, array('solr_profile' => $name));

        // set attributes variable for remove link
        $attr = array();
        // set title
        $attr['title'] = $secondary_display_profiles[$name]['description'];
        // set rel
        $attr['rel'] = 'nofollow';
        // set url
        $attr['href'] = url($path, array('query' => $query_secondary));
        // logo
        $logo = $secondary_display_profiles[$name]['logo'];
        // create link
        // we're not using l() because of active classes: http://drupal.org/node/41595
        $query_list[] = '<a' . drupal_attributes($attr) . '>' . $logo . '</a>';
      }
    }

    return theme('item_list', array('items' => $query_list, 'title' => NULL, 'type' => 'ul', 'attributes' => array('id' => 'secondary-display-profiles')));
  }

  /**
   * Renders the primary or secondary display profile based on the raw Solr
   * results. This is the method most Islandora Solr display plugins would
   * override.
   *
   * @param type $results
   *   The raw Solr results from IslandoraSolrQueryProcessor::islandoraSolrResult
   *
   * @return
   *   Rendered Solr results
   *
   * @see IslandoraSolrResults::displayResults()
   */
  function printResults($solr_results) {
    $solr_results = islandora_solr_prepare_solr_results($solr_results);
    $object_results = $solr_results['response']['objects'];
    $object_results = islandora_solr_prepare_solr_doc($object_results);
    $elements = array();
    // total
    $elements['solr_total'] = $solr_results['response']['numFound'];
    // start
    $elements['solr_start'] = $solr_results['response']['start'];
    // Return themed search results.
    return theme('islandora_solr', array('results' => $object_results, 'elements' => $elements));
  }


  /**
   * Displays elements of the current solr query. Displays current query and
   * current filters. Includes a link to exclude the query/filter.
   *
   * @param IslandoraSolrQueryProcessor $islandoraSolrQuery
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return
   *   Rendered lists of the currently active query and/or filters.
   */
  function currentQuery($islandoraSolrQuery) {
    // set path
    $path = SOLR_SEARCH_PATH . '/' . replace_slashes($islandoraSolrQuery->solrQuery);
    // get date format
    $format = variable_get('islandora_solr_facet_date_format', 'Y');
    // set output variable
    $output = '';
    // get user provided filter parameters
    $fq = isset($islandoraSolrQuery->internalSolrParams['f']) ? $islandoraSolrQuery->internalSolrParams['f'] : array();
    // parameters set in url
    $params = $islandoraSolrQuery->internalSolrParams;
    // Get Query values
    if (!in_array($islandoraSolrQuery->solrQuery, $islandoraSolrQuery->different_kinds_of_nothing)) {
      // get query value
      $query_value = $islandoraSolrQuery->solrQuery;
      // set list variables
      $query_list = array();
      // remove link keeps all parameters (query gets removed instead)
      $query_minus = array();
      $query_minus = $params;

      // remove query from path
      $path_minus = implode('/', explode('/', $path, -1)) . '/ ';

      // set attributes variable for remove link
      $attr_minus = array();
      // set title
      $attr_minus['title'] = t('Remove') . ' ' . $query_value;
      // set class
      $attr_minus['class'] = 'remove-query';
      // set rel
      $attr_minus['rel'] = 'nofollow';
      // set url
      $attr_minus['href'] = url($path_minus, array('query' => $query_minus));

      // create link
      // we're not using l() because of active classes: http://drupal.org/node/41595
      $query_list[] = '<a' . drupal_attributes($attr_minus) . '>(-)</a> ' . check_plain($query_value);

      // add wrap and list
      $output .= '<div class="islandora-solr-query-wrap">';
      $output .= theme('item_list', array('items' => $query_list, 'title' => t('Query'), 'type' => 'ul', 'attributes' => array('class' => 'islandora-solr-query-list query-list')));
      $output .= '</div>';

    }

    // Get Filter values
    if (!empty($fq)) {
      // set list variables
      $filter_list = array();
      // loop over filters
      foreach ($fq as $key => $filter) {

        // check for exclude filter
        if ($filter[0] == '-') {
          // not equal sign
          $symbol = '&ne;';
        }
        else {
          $symbol = '=';
        }
        $filter_string = $this->formatFilter($filter, $islandoraSolrQuery);
        // pull out filter (for exclude link)
        $query_minus = array();
        $f_x['f'] = array_diff($params['f'], array($filter));
        $query_minus = array_merge($params, $f_x);
        // resetting the filter key's order // @TODO find a cleaner way to do this.
        if ($query_minus['f']) {
          $query_minus['f'] = array_merge(array(), $query_minus['f']);
        }
        // remove 'f' if empty
        if (empty($query_minus['f'])) {
          unset($query_minus['f']);
        }
        // set attributes variable for remove link
        $attr_minus = array();
        // set title
        $attr_minus['title'] = t('Remove') . ' ' . $filter;
        // set class
        $attr_minus['class'] = 'remove-filter';
        // set rel
        $attr_minus['rel'] = 'nofollow';
        // set url
        $attr_minus['href'] = url($path, array('query' => $query_minus));
        // create link
        // we're not using l() because of active classes: http://drupal.org/node/41595
        $filter_list[] = '<a' . drupal_attributes($attr_minus) . '>(-)</a> ' . $symbol . ' ' . check_plain($filter_string);
      }
      // return filter list
      $output .= '<div class="islandora-solr-filter-wrap">';
      $output .= theme('item_list', array('items' => $filter_list, 'title' => t("Enabled Filters"), 'type' => 'ul', 'attributes' => array('class' => 'islandora-solr-filter-list filter-list')));
      $output .= '</div>';
    }
    return $output;
  }

  /**
   * Sets the drupal breadcrumbs based on the current query and filters.
   * Provides links to exclude the query or filters.
   *
   * @param type $islandoraSolrQuery
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   */
  function setBreadcrumbs($islandoraSolrQuery) {
    // set path
    $path = SOLR_SEARCH_PATH . '/' . replace_slashes($islandoraSolrQuery->solrQuery); // $_GET['q'] didn't seem to work here
    // get date format
    $format = variable_get('islandora_solr_facet_date_format', 'Y');
    // set breadcrumb variable
    $breadcrumb = array();
    // get user provided filter parameters
    if (isset($islandoraSolrQuery->internalSolrParams['f'])) {
      $fq = $islandoraSolrQuery->internalSolrParams['f'];
    }
    else {
      $fq = array();
    }

    // parameters set in url
    $params = $islandoraSolrQuery->internalSolrParams;
    // set filter key if there are no filters included
    if (empty($params['f'])) {
      $params['f'] = array();
    }

    // loop to create filter breadcrumbs if available
    if (!empty($fq)) {
      // set var
      $f['f'] = array();
      foreach ($fq as $key => $filter) {
        // check for exclude filter
        $exclude = FALSE;
        if ($filter[0] == '-') {
          $exclude = TRUE;
        }
        $filter_string = $this->formatFilter($filter, $islandoraSolrQuery);
        // increment filter array with current filter (for breadcrumb link)
        $query = array();
        $query_diff = array_diff($params, array('f' => array()));
        $f = array_merge_recursive($f, array('f' => array($filter)));
        $query = array_merge($query_diff, $f);

        // pull out filter (for x link)
        $query_x = array();
        $f_x['f'] = array_diff($params['f'], array($filter));
        $query_x = array_merge($params, $f_x);
        // resetting the filter key's order // @TODO find a cleaner way to do this.
        if ($query_x['f']) {
          $query_x['f'] = array_merge(array(), $query_x['f']);
        }
        // remove 'f' if empty
        if (empty($query_x['f'])) {
          unset($query_x['f']);
        }

        // set attributes variable
        $attr = array();
        // set title
        $attr['title'] = $filter;
        // set rel
        $attr['rel'] = 'nofollow';
        // exclude filter: add class "strikethrough"
        if ($exclude) {
          $attr['class'] = 'strikethrough';
        }
        // set url
        $attr['href'] = url($path, array('query' => $query));

        // set attributes variable for remove link
        $attr_x = array();
        // set title
        $attr_x['title'] = t('Remove') . ' ' . $filter;
        // set rel
        $attr_x['rel'] = 'nofollow';
        // set url
        $attr_x['href'] = url($path, array('query' => $query_x));
        // create links
        // we're not using l() because of active classes: http://drupal.org/node/41595
        $breadcrumb[] = '<a' . drupal_attributes($attr) . '>' . check_plain($filter_string) . '</a>'
              . '<span class="islandora-solr-breadcrumb-super"> <a' . drupal_attributes($attr_x) . '>(' . t('x') . ')</a></span>';

      }
      // at this point reverse the breadcrumbs array (only contains filters)
      $breadcrumb = array_reverse($breadcrumb);
    }

    // create Query breadcrumb
    if (!in_array($islandoraSolrQuery->solrQuery, $islandoraSolrQuery->different_kinds_of_nothing)) {
      // get query value
      $query_value = $islandoraSolrQuery->solrQuery;
      // remove all filters for this breadcrumb
      $query = array();
      $query = array_diff($params, array('f' => array()));
      // remove button keeps all parameters (query gets removed instead)
      $query_x = array();
      $query_x = $params;
      if (empty($params['f'])) {
        unset($query_x['f']);
      }
      // remove query from path
      $path_x = implode('/', explode('/', $path, -1)) . '/ ';
      // set attributes variable
      $attr = array();
      // set title
      $attr['title'] = $query_value;
      // set rel
      $attr['rel'] = 'nofollow';
      // set url
      $attr['href'] = url($path, array('query' => $query));

      // set attributes variable for remove link
      $attr_x = array();
      // set title
      $attr_x['title'] = t('Remove') . ' ' . $query_value;
      // set rel
      $attr_x['rel'] = 'nofollow';
      // set url
      $attr_x['href'] = url($path_x, array('query' => $query_x));

      // remove solr fields from breadcrumb value
      $query_explode = explode(' ', $query_value);
      $query_implode = array();
      foreach ($query_explode as $value) {
        // check for first colon to split the string
        if (strpos($value, ':') != FALSE) {
          // split the filter into field and value
          $value_split = explode(':', $value, 2);
          // trim whitespace
          $value_split[1] = trim($value_split[1]);
          // trim brackets
          $value = str_replace(array('(', ')'), '', $value_split[1]);
        }
        // no colon is found
        else {
          $value = trim($value);
          // strip brackets
          $value = str_replace(array('(', ')'), '', $value);
        }
        $query_implode[] = $value;
      }
      $query_value = implode(" ", $query_implode);

      // create links
      // we're not using l() because of active classes: http://drupal.org/node/41595
      $breadcrumb[] = '<a' . drupal_attributes($attr) . '>' . check_plain($query_value) . '</a>'
            . '<span class="islandora-solr-breadcrumb-super"> <a' . drupal_attributes($attr_x) . '>(' . t('x') . ')</a></span>';
    }

    $breadcrumb[] = l(t('Home'), '<front>', array('attributes' => array('title' => t('Home'))));
    if (!empty($breadcrumb))
      $breadcrumb = array_reverse($breadcrumb);
    drupal_set_breadcrumb($breadcrumb);

  }

  /**
   * Function that formats the passed in filter into a human readable form.
   *
   * @param string $filter
   *   The passed in filter.
   * @param object $islandoraSolrQuery
   *   The current Solr Query
   * @return string
   *   The formatted filter string for breadcrumbs and active query.
   */
  function formatFilter($filter, $islandoraSolrQuery) {
    // Check if there are operators in the filter.
    // @TODO: See how this interacts with multiple date filters.
    $fq_split = preg_split('/ (OR|AND) /', $filter);
    if (count($fq_split) > 1) {
      $operator_split = explode(' ', $filter);
      $operator_split = array_diff($operator_split, $fq_split);
      $out_array = array();
      foreach ($fq_split as $fil) {
        $fil_split = explode(':', $fil, 2);
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
      // split the filter into field and value
      $filter_split = explode(':', $filter, 2);
      // trim brackets
      $filter_split[1] = trim($filter_split[1], "\"");
      // if value is date
      if (isset($islandoraSolrQuery->solrParams['facet.date']) && in_array(ltrim($filter_split[0], '-'), $islandoraSolrQuery->solrParams['facet.date'])) {
        // check date format setting
        foreach ($this->rangeFacets as $value) {
          if ($value['solr_field'] == $filter_split[0] && isset($value['solr_field_settings']['date_facet_format']) && !empty($value['solr_field_settings']['date_facet_format'])) {
            $format = $value['solr_field_settings']['date_facet_format'];
          }
        }
        // split range filter string to return formatted date values
        $filter_str = $filter_split[1];
        $filter_str = trim($filter_str, '[');
        $filter_str = trim($filter_str, ']');
        $filter_array = explode(' TO ', $filter_str);
        $filter_split[1] = format_date(strtotime(trim($filter_array[0])) + (60 * 60 * 24), 'custom', $format) . ' - ' . format_date(strtotime(trim($filter_array[1])) + (60 * 60 * 24), 'custom', $format);
      }
      $filter_string = $filter_split[1];
    }
    return $filter_string;
  }

  /**
   * Displays facets based on a query response. Includes links to include or
   * exclude a facet field in a search.
   *
   * @param type $islandoraSolrQuery
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   * @return
   *   Rendered lists of facets including links to include or exclude a facet
   *   field.
   *
   * @see islandora_solr_islandora_solr_query_blocks()
   * @see islandora_solr_block_view()
   *
   * // @TODO: clean this entire method up. It's really messy. Including _addFacets().
   */
  function displayFacets($islandoraSolrQuery) {
    IslandoraSolrFacets::init($islandoraSolrQuery);
    $output = '';
    $facet_order = $this->facetFieldArray;

    foreach ($facet_order as $facet_key => $facet_label) {
      $facet_obj = new IslandoraSolrFacets($facet_key);
      $output .= $facet_obj->getFacet();
    }

    // As we add additional facets, we're repeatedly URL-encoding old facet
    // strings. when we double-encode quotation marks they're incomprehensible
    // to solr. This is a quick workaround:                                                      //
    $output = str_replace('%2B', '%252B', $output);

    return $output;
  }

  /**
   * Creates a fieldset containing raw Solr results of the current page for
   * debugging purposes.
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $islandora_solr_result
   *   The processed Solr results from
   *   IslandoraSolrQueryProcessor::islandoraSolrResult
   *
   * @return
   *   Rendered fieldset containing raw Solr results data.
   */
  function printDebugOutput($islandora_solr_results) {
    // debug dump

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
   * Reads configuration values and preps a number of key => value arrays for
   * output substitution. Replaces solr field labels with human readable labels
   * as set in the admin form.
   */
  function prepFieldSubstitutions() {

    $this->facetFieldArray = islandora_solr_get_fields('facet_fields');

    $this->searchFieldArray = islandora_solr_get_fields('search_fields');

    $this->resultFieldArray = islandora_solr_get_fields('result_fields');

    $this->allSubsArray = array_merge($this->facetFieldArray, $this->searchFieldArray, $this->resultFieldArray);
  }
}

/**
 * Islandora Solr Facets
 */
class IslandoraSolrFacets {
  public static $islandoraSolrQuery;
  public static $facet_fields; // normal facet results
  public static $facet_dates; // date range facet results (Solr 1.4)
  public static $facet_ranges; // date or integer range facet results (Solr 3.1)

  public static $facet_fields_settings;
  public static $facet_fields_settings_simple;
  public static $range_facets; // rename?
  public static $minimum_count = 1;
  public static $soft_limit;
  public static $exclude_range_values;
  public static $needed_solr_call;
  public static $range_slider_key = 0;
  public static $date_filter_key = 0;

  public $facet_field;
  public $settings;
  public $facet_type;
  public $results;
  public $title = NULL;
  public $content = NULL;

  /**
   * Constructor method. Stores the facet field name, settings and title in a parameter.
   *
   * @param string $facet_field
   *   The name of the solr field to build a facet for.
   */
  function IslandoraSolrFacets($facet_field) {
    $this->facet_field = $facet_field;
    $this->settings = self::$facet_fields_settings[$facet_field];
    $this->title = self::$facet_fields_settings_simple[$facet_field];
  }

  /**
   * Static init.
   *
   * Populates static variables with Solr query results and user settings.
   *
   * @param object $islandoraSolrQuery
   *   Object containing the Solr query and results.
   */
  public static function init($islandoraSolrQuery) {
    self::$islandoraSolrQuery = $islandoraSolrQuery;
    self::$facet_fields = $islandoraSolrQuery->islandoraSolrResult['facet_counts']['facet_fields'];
    self::$facet_dates = $islandoraSolrQuery->islandoraSolrResult['facet_counts']['facet_dates'];
    self::$facet_ranges = $islandoraSolrQuery->islandoraSolrResult['facet_counts']['facet_ranges']; // not in place yet.

    self::$facet_fields_settings = islandora_solr_get_fields('facet_fields', TRUE, FALSE, TRUE); // filtered, not simplified and fields as keys
    self::$facet_fields_settings_simple = _islandora_solr_simplify_fields(self::$facet_fields_settings);
    self::$range_facets = islandora_solr_get_range_facets();
    self::$soft_limit = variable_get('islandora_solr_facet_soft_limit', '10');
    self::$exclude_range_values = array('gap', 'start', 'end', 'other', 'hardend', 'include');

    // Calculate variable date gap.
    // @XXX move elsewhere?
    self::variableDateGap();
  }

  /**
   * Prepare and render facet.
   *
   * Method called after a facet object is created. This will prepare the
   * results based on the type and user settings for this facet. It also does a
   * call to render the prepared data. This method also returns the rendered
   * endresult.
   *
   * @return string
   *   Returns the title and rendered facet.
   */
  public function getFacet() {
    $this->findFacetType();
    $this->getFacetResults();
    if (empty($this->results)) {
      return;
    }
    $this->processFacets();
    if (empty($this->content)) {
      return;
    }
    $elements = array(
      'title' => $this->title,
      'content' => $this->content,
      'pid' => $this->facet_field,
    );
    return theme('islandora_solr_facet_wrapper', $elements);
  }

  /**
   * Check facet type
   *
   * Depending on facet type and Solr version, Solr will return different types of facets in
   * different arrays in the results object. Itereate where the current facet object has
   * returned.
   */
  public function findFacetType() {
    $facet_field = $this->facet_field;
    // facet_fields
    if (array_key_exists($facet_field, self::$facet_fields)) {
      $this->facet_type = 'facet_fields';
    }
    // facet_dates
    if (array_key_exists($facet_field, self::$facet_dates)) {
      $this->facet_type = 'facet_dates';
    }
    // facet_ranges (not in place yet)
    if (array_key_exists($facet_field, self::$facet_ranges)) {
      $this->facet_type = 'facet_ranges';
    }
  }

  /**
   * Gets facet results.
   *
   * Finds and stores facet results from the Solr query into a property.
   */
  public function getFacetResults() {
    $facet_field = $this->facet_field;
    $facet_type = $this->facet_type;
    // facet_fields
    if ($facet_type == 'facet_fields') {
      $this->results = self::$facet_fields[$facet_field];
    }
    // facet_dates
    if ($facet_type == 'facet_dates') {
      $this->results = self::$facet_dates[$facet_field];
    }
    // facet_ranges (not in place yet)
    if ($facet_type == 'facet_ranges') {
      $this->results = self::$facet_ranges[$facet_field];
    }
  }

  /**
   * Process facets.
   *
   * Calls a process method based on the facet type. This method exists so this
   * process of preparing and rendering arrays can be started from one simple method
   * call.
   */
  public function processFacets() {
    $facet_type = $this->facet_type;
    // facet_fields
    if ($facet_type == 'facet_fields') {
      $this->processFacetFields();
    }
    // facet_dates
    if ($facet_type == 'facet_dates') {
      $this->processFacetDates();
    }
    // facet_ranges (not in place yet)
    if ($facet_type == 'facet_ranges') {
      $this->processFacetRanges();
    }
  }

  /**
   * Process facet fields.
   *
   * Calls prepare method to prepare the array to be rendered. Then calls
   * the method to render the prepared results.
   */
  public function processFacetFields() {
    $prepared_facet_fields = $this->prepareFacetFields();
    $this->renderText($prepared_facet_fields);
  }

  /**
   * Process facet dates.
   *
   * Depending on the way the facets need to be rendered as set in the user configuration,
   * this method calls methods to prepare the results to be rendered. Then calls the
   * method to render the prepared results.
   * Currently there are 3 ways facet dates can be rendered: normal text, slider and/or
   * datepicker.
   * @TODO maybe there should be some better pluggable way to prepare and render facets so that contrib modules can create their own widgets.
   */
  public function processFacetDates() {
    // @TODO: make this check better. Could be pluggable in the future.
    if (isset($this->settings['solr_field_settings']['range_facet_slider_enabled']) &&
        $this->settings['solr_field_settings']['range_facet_slider_enabled'] == 1) {
      // prepare and render facet dates as slider.
      $facet_dates_as_slider = $this->prepareFacetDatesSlider();
      if (!empty($facet_dates_as_slider)) {
        $this->renderSlider($facet_dates_as_slider);
      }
    }
    else {
      // prepare and render facet dates as text.
      $facet_dates_as_text = $this->prepareFacetDates();
      $this->renderText($facet_dates_as_text);
    }
    // date filter
    if (isset($this->settings['solr_field_settings']['date_filter_datepicker_enabled']) &&
        $this->settings['solr_field_settings']['date_filter_datepicker_enabled'] == 1) {
      // prepare and render facet dates as slider.
      $facet_dates_datefilter = $this->prepareFacetDatesFilter();
      if (!empty($facet_dates_datefilter)) {
        $this->renderFacetDatesFilter($facet_dates_datefilter);
      }
    }
  }

  /**
   * Process facet ranges.
   *
   * Not in place yet.
   */
  public function processFacetRanges() {

  }

  /**
   * Get date format.
   *
   * Finds the date formatting settings from user configuration.
   */
  public function getDateFormat() {
    if (isset($this->settings['solr_field_settings']['date_facet_format']) &&
        !empty($this->settings['solr_field_settings']['date_facet_format'])) {
      return $this->settings['solr_field_settings']['date_facet_format'];
    }
    else {
      return 'Y';
    }
  }

  /**
   * Prepare facet fields for text rendering.
   */
  public function prepareFacetFields() {
    $results = $this->results;
    $facet_field = $this->facet_field;
    $facet_results = array();
    foreach ($results as $bucket => $count) {
      $facet_results[] = array(
        'count' => $count,
        'filter' => $facet_field . ':"' . $bucket . '"', // PID:"foo"
        'bucket' => $bucket,
      );
    }
    return $facet_results;
  }

  /**
   * Prepare facet dates for text rendering.
   */
  public function prepareFacetDates() {
    $facet_field = $this->facet_field;
    $results = $this->results;
    $format = self::getDateFormat();
    $date_results = array();
    // render date facet fields
    foreach ($results as $bucket => $count) {
      // don't include gap, end, etc that comes with range results
      if (in_array($bucket, self::$exclude_range_values)) {
        continue;
      }
      $item = array();
      // set count or documents
      $item['count'] = $count;
      // logic to get the next range key (next date)
      $field_keys = array_keys($results);
      $field_key = array_search($bucket, $field_keys);
      $field_key++;
      $bucket_next = (!in_array($field_keys[$field_key], self::$exclude_range_values)) ? $field_keys[$field_key] : $results['end'];
      // set date range filter for facet url
      $item['filter'] = $facet_field . ':[' . $bucket . ' TO ' . $bucket_next . ']';
      // set formatted value for facet link
      $item['bucket'] = format_date(strtotime($bucket) + (60 * 60 * 24), 'custom', $format) . ' - ' . format_date(strtotime($bucket_next) + (60 * 60 * 24), 'custom', $format);
      $date_results[] = $item;
    }
    return $date_results;
  }

  /**
   * Render text facets.
   *
   * Based on a prepared array of results, this method will process and render a facet
   * as normal text. It includes bucket value, count, include link and exclude link. If
   * configured it also adds a 'read more' link to expose more results.
   *
   * @param array $results
   *   An array with the prepared facet results.
   */
  public function renderText($results) {
    $facet_field = $this->facet_field;
    $islandoraSolrQuery = self::$islandoraSolrQuery;
    $soft_limit = self::$soft_limit;
    $buckets = array();
    foreach ($results as $key => $values) {
      $bucket = $values['bucket'];
      $filter = $values['filter'];
      $count = $values['count'];
      // current url query
      $fq = isset($islandoraSolrQuery->solrParams['fq']) ? $islandoraSolrQuery->solrParams['fq'] : array();
      // 1. check minimum count
      // 2. check if the filter isn't active
      if ($count < self::$minimum_count || array_search($filter, $fq) !== FALSE) {
        continue;
      }
      // current path including query, for example islandora/solr/query
      $path = SOLR_SEARCH_PATH . '/' . replace_slashes($islandoraSolrQuery->solrQuery); // $_GET['q'] didn't seem to work here
      // parameters set in url
      $params = $islandoraSolrQuery->internalSolrParams;
      // set filter key if there are no filters included
      if (!isset($params['f'])) {
        $params['f'] = array();
      }
      // merge recursively to add new filter parameter
      $query_plus = array_merge_recursive($params, array('f' => array($filter)));
      $query_minus = array_merge_recursive($params, array('f' => array('-' . $filter)));
      // set basic attributes
      $attr_plus = $attr_minus = $attr = array('rel' => 'nofollow');
       // add url
      $attr['href'] = $attr_plus['href'] = url($path, array('query' => $query_plus));
      $attr_minus['href'] = url($path, array('query' => $query_minus));
        // add classes
      $attr_plus['class'] = 'plus';
      $attr_minus['class'] = 'minus';
      // create links
      // we're not using l() because of active classes: http://drupal.org/node/41595
      $link['link'] = '<a' . drupal_attributes($attr) . '>' . $bucket . '</a>';
      $link['count'] = $count;
      $link['link_plus'] = '<a' . drupal_attributes($attr_plus) . '>+</a>';
      $link['link_minus'] = '<a' . drupal_attributes($attr_minus) . '>-</a>';
      $buckets[] = $link;
    }

    // Show more link.
    if (count($buckets) > $soft_limit) {
      $buckets_visible = array_slice($buckets, 0, $soft_limit);
      $buckets_hidden = array_slice($buckets, $soft_limit);
      $this->content .= theme('islandora_solr_facet', array('buckets' => $buckets_visible, 'hidden' => FALSE, 'pid' => $facet_field));
      $this->content .= theme('islandora_solr_facet', array('buckets' => $buckets_hidden, 'hidden' => TRUE, 'pid' => $facet_field));
      $this->content .= $this->showMore();
    }
    elseif (!empty($buckets)) {
      $this->content .= theme('islandora_solr_facet', array('buckets' => $buckets, 'hidden' => FALSE, 'pid' => $facet_field));
    }
  }

  /**
   * Adds a 'show more' link.
   *
   * @return string
   *   Returns a rendered 'show more' link.
   */
  public function showMore() {
    drupal_add_js(drupal_get_path('module', 'islandora_solr') . '/js/islandora_solr_facets.js');
    $show_more = '<a href="#" class="soft-limit">' . t('Show more') . '</a>';
    return $show_more;
  }

  /**
   * Prepare facet dates for slider
   *
   * Preparing this array includes:
   * - Stripping dates highest and lowest end that have no results when variable range gap is used.
   * - Calculate the range gap based on the first two results.
   * - Based on the calculation, name the range and assign date formatting.
   * - Add color as set in user configuration.
   */
  public function prepareFacetDatesSlider() {
    $facet_field = $this->facet_field;
    $settings = $this->settings;
    $results = $this->results;
    $format = self::getDateFormat();
    $needed_solr_call = self::$needed_solr_call;
    $range_slider_key = self::$range_slider_key;

    $date_results = array();
    // grab gap and end, and strip all non-buckets in results
    $results_gap = $results['gap'];
    $results_end = $results['end'];
    foreach (self::$exclude_range_values as $exclude) {
      unset($results[$exclude]);
    }

    // strip empty buckets top and bottom when no date range filters are set
    if (!in_array($facet_field, $needed_solr_call)) {
      // strip top
      foreach ($results as $bucket => $count) {
        if ($count == 0) {
          unset($results[$bucket]);
        }
        else {
          break;
        }
      }
      // reverse and strip other side
      $results = array_reverse($results);
      $new_end = array();
      foreach ($results as $bucket => $count) {
        if ($count == 0) {
          unset($results[$bucket]);
          $new_end = array('bucket' => $bucket, 'count' => $count);
        }
        else {
          break;
        }
      }
      // reverse to normal order
      $results = array_reverse($results);

      // add end date
      if (isset($new_end['count']) AND $new_end['count'] == 0) {
        $end_bucket = $new_end['bucket'];
        $results[$end_bucket] = NULL;
      }
      else {
        $results[$results_end] = NULL;
      }
    }
    // do not strip empty buckets left and right when filters are set
    else {
      // add end date
      $results[$results_end] = NULL;
    }

    // if values are available
    if (count($results) <= 1) {
      return array();
    }

    // calculate gap
    $calc_from = strtotime(key($results));
    next($results);
    $calc_to = strtotime(key($results));
    // calculate difference between from and to date
    $calc_diff = abs($calc_from - $calc_to);

    // total difference in days
    $calc_total_days = floor($calc_diff / 60 / 60 / 24);

    // get gap based on total days diff
    // @TODO: could be done nicer?
    // @TODO: date_format configurable.
    $gap = NULL;
    $date_format = 'Y';
    if ($calc_total_days < 7) {
      $gap = t('days');
      $date_format = 'M j, Y';
    }
    elseif ($calc_total_days >= 7 && $calc_total_days <= 28) {
      $gap = t('weeks');
      $date_format = 'M j, Y';
    }
    elseif ($calc_total_days >= 28 && $calc_total_days <= 32) {
      $gap = t('months');
      $date_format = 'M Y';
    }
    elseif ($calc_total_days >= 360 && $calc_total_days <= 370) {
      $gap = t('years');
      $date_format = 'Y';
    }
    elseif ($calc_total_days >= 720 && $calc_total_days <= 740) {
      $gap = t('2 years');
      $date_format = 'Y';
    }
    elseif ($calc_total_days >= 1800 && $calc_total_days <= 1850) {
      $gap = t('5 years');
      $date_format = 'Y';
    }
    elseif ($calc_total_days >= 3600 && $calc_total_days <= 3700) {
      $gap = t('decades');
      $date_format = 'Y';
    }
    elseif ($calc_total_days >= 36000 && $calc_total_days <= 37000) {
      $gap = t('centuries');
      $date_format = 'Y';
    }
    elseif ($calc_total_days >= 360000 && $calc_total_days <= 370000) {
      $gap = t('millennia');
      $date_format = 'Y';
    }

    // create a nice array with our data
    $data = array();
    foreach ($results as $bucket => $count) {
      $bucket_formatted = format_date(strtotime(trim($bucket)) + 1, 'custom', $date_format, 'UTC');

      $bucket_formatted = str_replace(' ', '&nbsp;', $bucket_formatted);
      $data[] = array(
        'date' => $bucket,
        'bucket' => $bucket_formatted,
        'count' => $count,
      );
    }

    // add range slider color
    if (isset($settings['solr_field_settings']['range_facet_slider_color']) &&
        !empty($settings['solr_field_settings']['range_facet_slider_color'])) {
      $slider_color = $settings['solr_field_settings']['range_facet_slider_color'];
    }
    else {
      $slider_color = '#edc240';
    }

    $elements = array(
      'data' => $data,
      'facet_field' => $facet_field,
      'slider_color' => $slider_color,
      'gap' => $gap,
      'date_format' => $date_format,
      'form_key' => $range_slider_key,
    );
    self::$range_slider_key++;

    return $elements;
  }

  /**
   * Render slider.
   *
   * Based on the prepared results array, render the slider. The rendered slider is
   * a form. It passes the array and returns and renders the form.
   */
  public function renderSlider($facet_dates_as_slider) {
    $elements = $facet_dates_as_slider;
    $range_slider_form = drupal_get_form('islandora_solr_range_slider_form_' . $elements['form_key'], $elements);
    $this->content .= drupal_render($range_slider_form);
  }

  /**
   * Prepare facet dates filter
   *
   * Prepares an array with settings to render a date/range filter.
   */
  public function prepareFacetDatesFilter() {
    $settings = $this->settings;
    // datepicker range
    if (isset($settings['solr_field_settings']['date_filter_datepicker_range']) && !empty($settings['solr_field_settings']['date_filter_datepicker_range'])) {
      $datepicker_range = $settings['solr_field_settings']['date_filter_datepicker_range'];
    }
    else {
      $datepicker_range = '-100:+3';
    }
    $elements = array(
      'facet_field' => $this->facet_field,
      'datepicker_range' => $datepicker_range,
      'form_key' => self::$date_filter_key,
    );
    self::$date_filter_key++;

    return $elements;
  }

  /**
   * Renders a facet dates filter.
   *
   * Based on the prepared array with elements, this method passes the array to a form, returns it and
   * then renders the form.
   */
  public function renderFacetDatesFilter($elements) {
    $date_filter_key = self::$date_filter_key;
    $date_filter_form = drupal_get_form('islandora_solr_date_filter_form_' . $elements['form_key'], $elements);
    if (!empty($this->content)) {
      $this->content .= drupal_render($date_filter_form);
    }
  }

  /**
   * Variable date gap.
   *
   * This method calculates the upper and lower ranges of a date range facet. It also performs a second Solr
   * query execution if necessary to recalculate the date range facet with a new range gap.
   * This is a static function because it calculates multiple date range facets, so in case multiple facets need
   * recalculation, it can do this in one Solr call, which saves on performance. Statscomponent will be used
   * in the future for range facets in Solr 3.5 or higher.
   *
   * Tasks this method performs:
   * 1. gather info
   * 2. calculate and render range gap
   * 3. execute
   * 4. prepare for return
   * 5. update date facets
   */
  public static function variableDateGap() {
    // 1. gather info
    // check settings
    // @TODO: move this to separate function!
    $facet_fields_settings = self::$facet_fields_settings;
    $variable_date_gap = array();
    foreach ($facet_fields_settings as $key => $settings) {
      if (isset($settings['solr_field_settings']['range_facet_variable_gap']) && $settings['solr_field_settings']['range_facet_variable_gap'] == 1) {
        $variable_date_gap[] = $settings['solr_field'];
      }
    }

    $islandoraSolrQuery = self::$islandoraSolrQuery;
    $fq = isset($islandoraSolrQuery->solrParams['fq']) ? $islandoraSolrQuery->solrParams['fq'] : array();
    $facet_dates = self::$facet_dates;
    // populate with terms that needed a second solr call to update facets
    $needs_solr_call = array();
    // loop over all date facets
    foreach ($facet_dates as $solr_field => $buckets) {
      $values = array();
      // loop over all filters
      foreach ($fq as $key => $filter) {
        // check for enabled range filters
        if (strpos($filter, $solr_field) === FALSE) {
          continue;
        }
        // Don't include excluded ranges for now, because it's complicated.
        // Maybe keep it like this for Solr 1.4. Solr 3.5 and up would calculate this
        // through the StatsComponent.
        if (substr($filter, 0, 1) == '-') {
          continue;
        }
        // Check 'variable range gap' settings for this field.
        // @TODO: this could be done differently?
        if (!in_array($solr_field, $variable_date_gap)) {
          continue;
        }
        // split the filter into field and value
        $filter_split = explode(':', $filter, 2);
        // trim brackets
        $filter_split[1] = trim($filter_split[1], "\"");
        // split range filter string to return formatted date values
        $filter_str = $filter_split[1];
        $filter_str = trim($filter_str, '[');
        $filter_str = trim($filter_str, ']');
        $filter_array = explode(' TO ', $filter_str);
        // collect values in array: timestamp => ISO8601
        $from_str = trim($filter_array[0]);
        $to_str = trim($filter_array[1]);
        $field = trim($filter_split[0], ' -');
        // if a star is given, we need to perform an extra query to find out
        // what the minimum or maximum value is that 'star' would return.
        if (strpos($from_str, '*') !== FALSE) {
          $from_str = self::findMinMaxValue($field, 'asc');
        }
        if (strpos($to_str, '*') !== FALSE) {
          $to_str = self::findMinMaxValue($field, 'desc');
        }
        $values['from'][strtotime(strtolower($from_str))] = $from_str;
        $values['to'][strtotime(strtolower($to_str))] = $to_str;
      }

      // if the date facet field is found as at least one range filter, calculate gap
      if (empty($values)) {
        continue;
      }

      // from max value & to min value
      $from_unix = max(array_keys($values['from']));
      $to_unix = min(array_keys($values['to']));

      // if the from date is bigger than the to date, abort.
      if ($from_unix >= $to_unix) {
        continue;
      }

      // get ISO8601 values
      $from = $values['from'][$from_unix];
      // if the hour is 00:00:00, subtract one second. If we always subtract
      // one second we keep eating away seconds after every filter
      if (strpos($from, '00:00:00') !== FALSE) {
        $from .= '-1SECOND';
      }
      $to = $values['to'][$to_unix];

      // 2. calculate and render range gap
      // calculate difference between from and to date
      $diff = abs($from_unix - $to_unix);

      // total difference in days
      $total_days = floor($diff / 60 / 60 / 24);

      // for maximum 15 buckets
      // @TODO: make max buckets variable
      // @TODO: fine tune this: it's not very precise.
      if ($total_days <= 15) {
        $gap = '+1DAY';
      }
      elseif ($total_days <= 105) {
        $gap = '+7DAYS';
      }
      elseif ($total_days <= 450) {
        $gap = '+1MONTH';
      }
      elseif ($total_days <= 5475) {
        $gap = '+1YEAR';
      }
      elseif ($total_days <= 10950) {
        $gap = '+2YEARS';
      }
      elseif ($total_days <= 18250) {
        $gap = '+5YEARS';
      }
      elseif ($total_days <= 54750) {
        $gap = '+10YEARS';
      }
      elseif ($total_days <= 547500) {
        $gap = '+100YEARS';
      }
      elseif ($total_days <= 5475000) {
        $gap = '+1000YEARS';
      }

      // update range facet values
      // @TODO: try to find a way to clone this object, because it's passed by reference throughout the entire page call.
      $islandoraSolrQuery->solrParams["f.$solr_field.facet.date.start"] = $from;
      $islandoraSolrQuery->solrParams["f.$solr_field.facet.date.end"] = $to;
      $islandoraSolrQuery->solrParams["f.$solr_field.facet.date.gap"] = $gap;

      // update variable
      $needs_solr_call[] = $solr_field;
    }

    // 3. execute
    // if an extra solr call is necessary
    if (!empty($needs_solr_call)) {
      // new query processor class
      $rangeQuery = new IslandoraSolrQueryProcessor();

      // internal solr query
      $rangeQuery->internalSolrQuery = $islandoraSolrQuery->internalSolrQuery;
      // add solr limit
      $rangeQuery->solrLimit = 0;
      // add solr start
      $rangeQuery->solrStart = 0;
      // add query (defaults to *:*)
      $rangeQuery->solrQuery = $islandoraSolrQuery->solrQuery;
      // add solr parameters
      $rangeQuery->solrParams = $islandoraSolrQuery->solrParams;
      // no need to include normal facets
      unset($rangeQuery->solrParams['facet.field']);

      // excecute query
      $rangeQuery->executeQuery();

      // 4. run query.
      $response_array = $rangeQuery->islandoraSolrResult;

      // 5. update date facets
      self::$facet_dates = $response_array['facet_counts']['facet_dates'];
    }
    self::$needed_solr_call = $needs_solr_call;
  }

  /**
   * Finds the maximum or minimum value of a date field in a query. Called when
   * one of the values in a date range filter equals to '*'.
   *
   * @param string $field
   *   Solr field to sort on.
   * @param string $order
   *   sort order (asc or desc). Defaults to ascending.
   *
   * @return string
   *   maximum or minimum value of a date field in the current query
   */
  public static function findMinMaxValue($field, $order = 'asc') {
    $islandoraSolrQuery = self::$islandoraSolrQuery;

    // new query processor class
    $minMaxQuery = new IslandoraSolrQueryProcessor();
    $minMaxQuery->internalSolrQuery = $islandoraSolrQuery->internalSolrQuery;
    $minMaxQuery->solrLimit = 1;
    $minMaxQuery->solrStart = 0;
    $minMaxQuery->solrQuery = $islandoraSolrQuery->solrQuery;
    $minMaxQuery->solrParams = $islandoraSolrQuery->solrParams;
    // no need to include normal facets
    unset($minMaxQuery->solrParams['facet.field']);
    // add the right sorting
    $minMaxQuery->solrParams['sort'] = $field . ' ' . $order;
    $minMaxQuery->executeQuery();
    // solr results
    $results = $minMaxQuery->islandoraSolrResult;
    $response_arrayonse = $results['response'];

    return $response_array['response']['objects'][0]['solr_doc'][$field];
  }
}
