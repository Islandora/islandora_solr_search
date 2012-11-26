<?php

/**
 * @file
 * Contains methods to create rendered Solr displays from raw Solr results.
 * Depends on Apache_Solr_Php client.
 */

/**
 * Islandora Solr Results
 */
class IslandoraSolrResults {

  public $facetFieldArray = array();
  public $searchFieldArray = array();
  public $resultFieldArray = array();
  public $allSubsArray = array();

  /**
   * Constructor
   */
  function IslandoraSolrResults() {

    // include common.inc
    module_load_include('inc', 'islandora_solr', 'includes/common');
    $this->prepFieldSubstitutions();
  }

  /**
   * Output the main body of the search results
   *
   * @param type $islandoraSolrQuery
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   * @return
   *   Returns themed Solr results page, including wrapper and rendered search
   *   results.
   *
   * @see islandora_solr()
   */
  function displayResults($islandoraSolrQuery) {

    // set variables to collect returned data.
    $results = NULL;
    $secondary_profiles = NULL;
    $elements = array();

    // set breadcrumbs
    $this->setBreadcrumbs($islandoraSolrQuery);

    // raw solr results
    $apacheSolrResult = $islandoraSolrQuery->solrResult;

    // solr results count
    // total solr results
    $total = (int) $apacheSolrResult->response->numFound;
    $elements['solr_total'] = $total;

    // solr start
    // to display: $islandoraSolrQuery->solrStart + ($total > 0 ? 1 : 0)
    $elements['solr_start'] = $islandoraSolrQuery->solrStart;

    // solr results end
    $end = min(($islandoraSolrQuery->solrLimit + $islandoraSolrQuery->solrStart), $total);
    $elements['solr_end'] = $end;

    // pager
    islandora_solr_pager_init($total, $islandoraSolrQuery->solrLimit);
    $elements['solr_pager'] = theme('pager', array(NULL, $islandoraSolrQuery->solrLimit, 0, NULL, 5));

    // debug (will be removed)
    $elements['solr_debug'] = '';
    if (variable_get('islandora_solr_debug_mode', 0)) {
      $elements['solr_debug'] = $this->printDebugOutput($apacheSolrResult);
    }

    // rendered secondary display profiles
    $secondary_profiles = $this->addSecondaries($islandoraSolrQuery);

    // rendered results
    $results = $this->printResults($apacheSolrResult);

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
   *   The raw Solr results from IslandoraSolrQueryProcessor::solrResult
   * @return
   *   Rendered Solr results
   *
   * @see IslandoraSolrResults::displayResults()
   */
  function printResults($results) {
    // set variables
    $results_array = array();
    $elements = array();
    $pids = array();
    // total
    $elements['solr_total'] = $results->response->numFound;
    // start
    $elements['solr_start'] = $results->response->start;
    // get prepared search results
    $docs = $this->prep_results($results);
    // optionally limit results to values given
    $limit_results = variable_get('islandora_solr_limit_result_fields', 0);
    // look for fields with no permission
    $fields_all = islandora_solr_get_fields('result_fields', FALSE);
    $fields_filtered = islandora_solr_get_fields('result_fields');
    $fields_no_permission = array_diff($fields_all, $fields_filtered);

    // loop over results
    foreach ($docs as $doc) {
      $rows = array();
      $row = 0; // keeping $row in there for now until the snippet issue is cleared out
      $snippet = NULL; // @TODO: how is this suposed to work if the $snippet always is set to NULL ?
      $pids[] = $doc['PID'];
      $result_field_array = $this->resultFieldArray;

      // 1. add defined fields
      foreach ($result_field_array as $field => $label) {
        if (isset($doc[$field]) && !empty($doc[$field])) {
          $value = $doc[$field];
          $rows[$field]['label'] = $label;
          $rows[$field]['class'] = drupal_strtolower( preg_replace('/[^A-Za-z0-9]/', '-', $field));
          $rows[$field]['value'] = (is_array($value)) ? implode(", ", $value) : $value; // split if it's an array
        }
      }

      // 2. if limit isn't set, add other fields
      if ($limit_results == 0) {
        foreach ($doc as $field => $value) {
          // skip if added by the first loop already OR if no permission
          if (isset($rows[$field]) || in_array($field, $fields_no_permission)) {
            continue;
          }
          $rows[$field]['label'] = $field;
          $rows[$field]['class'] = drupal_strtolower( preg_replace('/[^A-Za-z0-9]/', '-', $field));
          $rows[$field]['value'] = (is_array($value)) ? implode(", ", $value) : $value; // split if it's an array
        }
      }
//      $row++; // keeping $row in there for now until the snippet issue is cleared out
      // @TODO: how is this suposed to work if the $snippet always is set to NULL ?
      if ($snippet) {
        $rows[$row][] = array(
          'data' => 'Full Text',
          'header' => TRUE,
        );
        $rows[$row][] = $snippet[0];
      }
      // append to results array
      $results_array[] = $rows;
    }

    // return themed search results
    return theme('islandora_solr', array('results' => $results_array, 'elements' => $elements, 'pids' => $pids));
  }


  /**
   * Displays elements of the current solr query. Displays current query and
   * current filters. Includes a link to exclude the query/filter.
   *
   * @param type $islandoraSolrQuery
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   * @return
   *   Rendered lists of the currently active query and/or filters.
   */
  function currentQuery($islandoraSolrQuery) {

    // set path
    $path = SOLR_SEARCH_PATH . '/' . replace_slashes($islandoraSolrQuery->solrQuery); // $_GET['q'] didn't seem to work here
    // get date format
    $format = variable_get('islandora_solr_facet_date_format', 'Y');
    // set output variable
    $output = '';
    // get user provided filter parameters
    $fq = isset($islandoraSolrQuery->internalSolrParams['f']) ? $islandoraSolrQuery->internalSolrParams['f'] : array();
    // parameters set in url
    $params = $islandoraSolrQuery->internalSolrParams;
    // get range facets
    $range_facets = islandora_solr_get_range_facets();


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
      $output .= '</div>'; // 'islandora_solr_query_wrap'

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

        // split the filter into field and value
        $filter_split = explode(':', $filter, 2);
        // trim brackets
        $filter_split[1] = trim($filter_split[1], "\"");

        // if value is date
        if (isset($islandoraSolrQuery->solrParams['facet.date']) && in_array(ltrim($filter_split[0], '-'), $islandoraSolrQuery->solrParams['facet.date'])) {
          // check date format setting
          foreach ($range_facets as $value) {
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
        $filter_list[] = '<a' . drupal_attributes($attr_minus) . '>(-)</a> ' . $symbol . ' ' . check_plain($filter_split[1]);
      }
      // return filter list
      $output .= '<div class="islandora-solr-filter-wrap">';
      $output .= theme('item_list', array('items' => $filter_list, 'title' => t("Enabled Filters"), 'type' => 'ul', 'attributes' => array('class' => 'islandora-solr-filter-list filter-list')));
      $output .= '</div>'; // 'islandora_solr_filter_wrap'
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
    // get range facets
    $range_facets = islandora_solr_get_range_facets();
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

        // split the filter into field and value
        $filter_split = explode(':', $filter, 2);
        // trim brackets
        $filter_split[1] = trim($filter_split[1], "\"");

        // if value is date
        if (isset($islandoraSolrQuery->solrParams['facet.date']) && in_array(ltrim($filter_split[0], '-'), $islandoraSolrQuery->solrParams['facet.date'])) {
          // check date format setting
          foreach ($range_facets as $value) {
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
        $breadcrumb[] = '<a' . drupal_attributes($attr) . '>' . check_plain($filter_split[1]) . '</a>'
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
    // set variables
    $facet_output = $output = '';
    $facet_min_count = 1;
    $min_result = variable_get('islandora_solr_facet_min_limit', '2');
    $initial_limit = variable_get('islandora_solr_facet_soft_limit', '10');
    $facet_limit = variable_get('islandora_solr_facet_max_limit', '20');
    $format = variable_get('islandora_solr_facet_date_format' , 'Y');
    $show_more = FALSE;
    $range_facets = islandora_solr_get_range_facets();

    // get facet values returned from solr
    $facets = isset($islandoraSolrQuery->solrResult->facet_counts->facet_fields) ? $islandoraSolrQuery->solrResult->facet_counts->facet_fields : array();
    $facets_json  = json_encode($facets);
    $facets = json_decode($facets_json, TRUE);

    // get date facet values
    $date_facets = isset($islandoraSolrQuery->solrResult->facet_counts->facet_dates) ? $islandoraSolrQuery->solrResult->facet_counts->facet_dates : array();
    $date_facets_json  = json_encode($date_facets);
    $date_facets_arr = json_decode($date_facets_json, TRUE);

    // loop over each date field // clean up facet dates array
    $date_facets = array();
    foreach ($date_facets_arr as $key => $fields) {
      $date_facets[$key] = array();
      // check date format setting
      foreach ($range_facets as $value) {
        if ($value['solr_field'] == $key && isset($value['solr_field_settings']['date_facet_format']) && !empty($value['solr_field_settings']['date_facet_format'])) {
          $format = $value['solr_field_settings']['date_facet_format'];
        }
      }
      // render date facet fields
      foreach ($fields as $date => $number) {
        // facet.date includes other values in the facet results - excluding these
        // we're turning the facet results in an array with multiple values
        if (!in_array($date, array('gap', 'end', 'other', 'hardend', 'include'))) {
          // set number or documents
          $date_facets[$key][$date]['number'] = $number;
          // logic to get the next range key (next date)
          $field_keys = array_keys($fields);
          $field_key = array_search($date, $field_keys);
          $field_key++;
          $date_next = (!in_array($field_keys[$field_key], array('gap', 'end', 'other', 'hardend', 'include'))) ? $field_keys[$field_key] : $fields['end'];
          // set date range filter for facet url
          $date_facets[$key][$date]['range'] = '[' . $date . ' TO ' . $date_next . ']';
          // set formatted value for facet link
          $date_facets[$key][$date]['value'] = format_date(strtotime($date) + (60 * 60 * 24), 'custom', $format) . ' - ' . format_date(strtotime($date_next) + (60 * 60 * 24), 'custom', $format);
        }
      }
    }

    // merge facet fields with date facets and order
    $facets_unordered = array_merge($date_facets, $facets);
    $facet_order = islandora_solr_get_fields('facet_fields');
    $facets = array();
    foreach ($facet_order as $term => $label) {
      $facets[$term] = $facets_unordered[$term];
    }

    // return if no facets are available
    if (empty($facets)) {
      return $output;
    }

    // loop over returned facet objects
    foreach ($facets as $facet_key => $facet) {
      $list_items = array();
      $list_title = NULL;

      // if the facet contains the minimum amount of values
      if (count($facet) >= $facet_min_count) {
        // set facet count
        $facet_count = 0;

        // render facet values
        foreach ($facet as $value => $data) {
          // add facet
          $this->_addFacets($facet_key, $value, $data, $islandoraSolrQuery, $facet_count, $list_items, $min_result);
        }

        // check for minumum returned values again, because some might have been disabled
        if (count($list_items) >= $facet_min_count) {

          // opening facet div // @TODO: this should go in theme function
          $facet_output .='<div class="islandora-solr-facet">';

          // set title
          $list_title = $this->facetFieldArray[$facet_key];

          // check if facet value amount is larger than the initial limit
          if ($facet_count > $initial_limit && $initial_limit < $facet_limit && $initial_limit > $facet_min_count) {
            $split_facet = TRUE;
          }
          else {
            $split_facet = FALSE;
          }

          if ($split_facet) {
            $show_more = TRUE; //There exists a split array...
            $list_items_hidden = array_slice($list_items, $initial_limit);
            $list_items = array_slice($list_items, 0, $initial_limit);
          }

          // theme the facet as a list
          $facet_output .= theme('item_list', array('items' => $list_items, 'title' => $list_title, 'type' => 'ul', 'attributes' => array('class' => 'facet-list')));

          // render the hidden facets
          if ($split_facet) {
            $facet_output .= '<a href="#" class="soft-limit">' . t('Show more') . '</a>';
            $facet_output .= '<a href="#" class="soft-limit hidden">' . t('Show less') . '</a>';
            $facet_output .= '<div class="toggle-wrapper hidden">';
            $facet_output .= theme('item_list', array('items' => $list_items_hidden, 'title' => NULL, 'type' => 'ul', 'attributes' => array('class' => 'facet-list')));
            $facet_output .= '</div>';
          }

          $facet_output .='</div>'; //div.islandora-solr-facet
        }
      }
    }

    if ($show_more) {
      // if there is at least one occurence of a split array, we add js to the page.
      drupal_add_js(drupal_get_path('module', 'islandora_solr') . '/js/islandora_solr_soft_limit.js');
    }

    // As we add additional facets, we're repeatedly URL-encoding old facet
    // strings. when we double-encode quotation marks they're incomprehensible
    // to solr. This is a quick workaround:                                                      //
    $facet_output = str_replace('%2B', '%252B', $facet_output);
    if (!empty($facet_output)) {
      $output .='<div class="islandora-solr-facet-wrap">';
      $output .= $facet_output;
      $output .= '</div>';
    }

    return $output;
  }


  /**
   * A facet callback. Renders a single facet field.
   *
   * @param type $key
   *   Facet field name.
   * @param type $value
   *   Facet field value.
   * @param type $number
   *   Facet value count. The amount of occurrences of the string in this
   *   specific Solr field in the index.
   * @param type $islandoraSolrQuery
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   * @param type $facet_count
   *   Incrementing ingeger to count fields per facet. Used to set a soft limit
   *   on facet fields, hiding a certain amount of facet fields by default and
   *   having the ability to expose more if the user chooses so.
   * @param array $list
   *   An array to populate all the fields per facet.
   *
   * @see IslandoraSolrResults::addFacets()
   */
  function _addFacets(&$key, &$value, &$data, &$islandoraSolrQuery, &$facet_count, &$list, $min_result) {

    // value of date facet is an array, normal facets is a string.
    if (is_array($data)) {
      // set value
      $value = $data['value'];
      // set filter to include in url
      $filter = $key . ':' . $data['range'];
      // set number of found solr documents
      $data = $data['number'];
      // don't include facet fields with a document count lower than the configured minimum
      if ($data < $min_result) {
        return;
      }
    }
    else {
      // @TODO: Should be changed to format_string for Drupal 7
      // @TODO: does this check for exclude filters? -PID:"foo"
      $filter = $key . ':"' . $value . '"'; // PID:"foo"
    }

    //FIXME:  This isn't quite right...  It can make sense to facet on
    //  the same field multiple times, provided it is multi-valued...

    $fq = isset($islandoraSolrQuery->solrParams['fq']) ? $islandoraSolrQuery->solrParams['fq'] : array();

    // check if item should show up
    // 1. check if the filter isn't active
    // 2. if the results of the facet items equals the total of current returned results, that means it's useless to include or exclude the filter.
    $disable_link = (array_search($filter, $fq) !== FALSE) || $data == $islandoraSolrQuery->solrResult->response->numFound;

    // if link isn't disabled
    if (!$disable_link) {

      // set variables
      $path = SOLR_SEARCH_PATH . '/' . replace_slashes($islandoraSolrQuery->solrQuery); // $_GET['q'] didn't seem to work here

      $query_plus = array();
      $query_minus = array();

      // parameters set in url
      $params = $islandoraSolrQuery->internalSolrParams;
      // set filter key if there are no filters included
      if (empty($params['f'])) {
        $params['f'] = array();
      }

      // merge recursively to add new filter parameter
      $query_plus = array_merge_recursive($params, array('f' => array($filter)));
      $query_minus = array_merge_recursive($params, array('f' => array('-' . $filter)));

      // set basic attributes
      $attr = array('rel' => 'nofollow');

      $attr_plus = $attr_minus = $attr;

      // add url
      $attr['href'] = url($path, array('query' => $query_plus));
      $attr_plus['href'] = url($path, array('query' => $query_plus));
      $attr_minus['href'] = url($path, array('query' => $query_minus));

      // add classes
      $attr_plus['class'] = 'plus';
      $attr_minus['class'] = 'minus';

      // create links
      // we're not using l() because of active classes: http://drupal.org/node/41595
      $list[] = '<a' . drupal_attributes($attr) . '>' . $value . '</a>' . ' (' . $data . ')' .
          '<span class="plusminus">' .
          '<a' . drupal_attributes($attr_plus) . '>+</a>'
          . ' ' .
          '<a' . drupal_attributes($attr_minus) . '>-</a>'
          . '</span>';

      $facet_count++;
    }
  }



  /**
   * Creates a fieldset containing raw Solr results of the current page for
   * debugging purposes.
   *
   * @param type $apacheSolrResults
   *   The raw Solr results from IslandoraSolrQueryProcessor::solrResult
   * @return
   *   Rendered fieldset containing raw Solr results data.
   *
   * @see IslandoraSolrResults::displayResults()
   */
  function printDebugOutput($apacheSolrResults) {
    // debug dump

    $results = "<pre>Results: " . print_r($apacheSolrResults, TRUE) . "</pre>";
    $fieldset = array(
      '#title' => t("Raw Results"),
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



  /**
   * Merges highlight fields with the rest of the solr results.
   *
   * @param type $results
   *   The raw Solr results from IslandoraSolrQueryProcessor::solrResult
   * @return
   *   raw Solr results array merged with highlight fields
   */
  function prep_results($results) {
    // catch if the result query returned an error
    if (!isset($results)) {
      return FALSE;
    }

    $raw_response = $results->getRawResponse();
    $response_array = json_decode($raw_response, TRUE);
//dsm($response_array);
    $highlights = isset($response_array['highlighting']) ? $response_array['highlighting'] : NULL;
    $docs = $response_array['response']['docs'];
//dsm($docs);
// @TODO: review/redo this.

/*
    if (!is_array($highlights)) {
      return $docs;
    }

    $snippet_fields = islandora_solr_get_snippet_fields();
   // if highights exist, inetegrate them back into the doc array
    $label = variable_get("islandora_solr_snippet_field", '');
    // blah~Blah,foo~Foo,bar~Bar
    if ($label) {
      $label_parts = explode('~', $label);
      $field_label = trim($label_parts[1]);
      $field_text = trim($label_parts[0]);
    }
    $this->resultFieldArray[$field_text] = $field_label;
    $this->allSubsArray[$field_text] = $field_label;
    foreach ($docs as &$doc) {
      $pid = $doc['PID'];
      unset($doc[$field_text]);
      $snippet = $highlights[$doc['PID']];
      foreach ($snippet as $key => $value) {
        $doc[$field_text] = implode('<br />', $value);
      }
    }
 */
    return $docs;
  }

}

