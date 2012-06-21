<?php

/**
 * @file
 * Contains methods to search solr and display results.  depends on Apache_Solr_Php client.
 */

/**
 * Islandora Solr Results
 */
class IslandoraSolrResults {

  public static $SEARCH_CLASS_ADVANCED_SEARCH_NUMBER_FIELDS = 5;
  public $facetFieldArray = array();
  public $searchFieldArray = array();
  public $resultFieldArray = array();
  public $allSubsArray = array();

  /**
   * Constructor
   */
  function IslandoraSolrResults() {
    if (module_exists('apachesolr')) {
      module_load_include('php', 'apachesolr', 'SolrPhpClient/Apache/Solr/Service');
    }
    else {
      module_load_include('php', 'islandora_solr_search', 'SolrPhpClient/Apache/Solr/Service');
    }
    module_load_include('inc', 'islandora_solr_search', 'includes/common');
    $this->prepFieldSubstitutions();
  }

  /**
   * Output the main body of the search results
   *
   * @param type $solrQueryProcessor
   * @param type $title
   * @param type $output
   * @return string
   */
  function displayResults($solrQueryProcessor, $title = NULL, $output = '') {
    
    // set variables to collect returned data.
    $results = NULL;
    $secondary_profiles = NULL;
    $elements = array();

    // set title
    if (!is_null($title)) {
      drupal_set_title($title);
    }
    
    // set breadcrumbs
    $this->setBreadcrumbs($solrQueryProcessor);
    
    // raw solr results
    $apacheSolrResult = $solrQueryProcessor->solrResult;
    
    // solr results count
    // total solr results
    $total = (int) $apacheSolrResult->response->numFound;
    $elements['solr_total'] = $total;

    // solr start
    // to display: $solrQueryProcessor->solrStart + ($total > 0 ? 1 : 0)
    $elements['solr_start'] = $solrQueryProcessor->solrStart;

    // solr results end
    $end = min(($solrQueryProcessor->solrLimit + $solrQueryProcessor->solrStart), $total);
    $elements['solr_end'] = $end;

    // pager
    islandora_solr_search_pager_init($total, $solrQueryProcessor->solrLimit);
    $elements['solr_pager'] = theme('pager', NULL, $solrQueryProcessor->solrLimit, 0, NULL, 5);

    // debug (will be removed)
    if (variable_get('islandora_solr_search_debug_mode', FALSE)) {
      $elements['solr_debug'] = $this->printDebugOutput($apacheSolrResult);
    }

    // rendered secondary display profiles
    $secondary_profiles = $this->addSecondaries($solrQueryProcessor);
    
    // rendered results
    $results = $this->printResults($apacheSolrResult);
        
    // return themed layout
    return theme('islandora_solr_wrapper', $results, $secondary_profiles, $elements);
  }

  function addSecondaries($solrQueryProcessor) {
    $query_list = array();
    // Get secondary display profiles
    $secondary_display_profiles = module_invoke_all('islandora_solr_secondary_display');
    
    // set path
    $path = SOLR_SEARCH_PATH . '/' . $solrQueryProcessor->solrQuery; // $_GET['q'] didn't seem to work here
    
    // parameters set in url
    $params = $solrQueryProcessor->internalSolrParams;
    
    // Expected return:
    // 
    // return array(
    // 'machine-name' = array(
    //   'name' => 'Human Readable Name',
    //   'module' => 'module_name',
    //   'file' => 'FileName.inc',
    //   'class' => 'ClassName',
    //   'function' => 'function_name',
    //   'description' => 'A description of the display profile',
    //   'logo' => 'Some text, some <html>, an <img>, etc used to link to this output,
    // );

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

    return theme('item_list', $query_list, NULL, "ul", array('class' => 'islandora-solr-search-secondary'));
  }

  /**
   * Function: printResults
   *
   * Description: translates a solr query result into a basic in-browser search result.
   *
   * @param Apache_Solr_Object $results
   */
  function printResults($results) {
    // set variables
    $results_array = array();
    $elements = array();
    
    // total
    $elements['solr_total'] = $results->response->numFound;

    // start
    $elements['solr_start'] = $results->response->start;
    
    // get prepared search results
    $docs = $this->prep_results($results);
    
    // optionally limit results to values given
    $limitResults = variable_get('islandora_solr_search_limit_result_fields', 0);

    // loop over results array
    foreach ($docs as $doc) {
      $rows = array();
      $row = 0; // keeping $row in there for now until the snippet issue is cleared out
      $snippet = NULL; // @TODO: how is this suposed to work if the $snippet always is set to NULL ?

      // loop over individual results to process fields
      foreach ($doc as $field => $value) {

        // check if this field should be included
        if ($limitResults && empty($this->resultFieldArray[$field])) {
          continue;
        }

        // check for field label substitutions
        $translated_field_name = isset($this->allSubsArray[$field]) ? $this->allSubsArray[$field] : $field;

        // add label
        $rows[$field]['label'] = $translated_field_name;

        // add class: render solr field name to string that can be used as class name
        $rows[$field]['class'] = strtolower( preg_replace('/[^A-Za-z0-9]/', '-', $field));

        // split value if the result value is an array
        if (is_array($value)) {
          $value = implode(", ", $value);
        }
        // add value
        $rows[$field]['value'] = $value;

        $row++; // keeping $row in there for now until the snippet issue is cleared out
      }
      // @TODO: how is this suposed to work if the $snippet always is set to NULL ?
      if ($snippet) {
        $rows[$row][] = array(
          'data' => 'Full Text',
          'header' => TRUE,
        );
        $rows[$row][] = $snippet[0];
      }
      // append array to results array
      $results_array[] = $rows;
    }

    // return themed search results
    return theme('islandora_solr_search', $results_array, $elements);
  }
  
  
  
  

  function printDebugOutput($results) {
    // debug dump
    $results_r .= "<pre>Results: " . print_r($results, TRUE) . "</pre>";
    $fieldset_r = array(
      '#title' => t("Raw Results"),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#value' => $results_r,
    );
    return theme('fieldset', $fieldset_r);
  }

  const FIELD_ALL = 0, FIELD_SEARCH = 1, FIELD_FACET = 2, FIELD_RESULT = 3;

  function _make_pattern($string) {
    return "/^" . preg_quote($string, '/') . "/";
  }

  function getSubstitutedFields($string_in, $type = FIELD_SEARCH) {
    $replacements = NULL;
    switch ($type) {
      case FIELD_SEARCH:
        $replacements = $this->searchFieldArray;
        break;
      case FIELD_FACET:
        $replacements = $this->facetFieldArray;
        break;
      case FIELD_RESULT:
        $replacements = $this->resultFieldArray;
        break;
      case FIELD_ALL:
      default:
        $replacements = $this->allSubsArray;
    }
    return preg_replace(array_map(array($this, '_make_pattern'), array_keys($replacements)), array_values($replacements), $string_in);
  }

  /**
   * Function: currentQuery
   *
   * Description: Displays elements of the current solr query (with enabled filters).
   *   This functionality was previously rolled into the facet block, but it makes
   *   sense to separate the two functions, particularly as the current query is now
   *   imprinted in the system's breadcrumb.
   *
   * @param IslandoraSolrQueryProcessor $solrQueryProcessor
   */
  function currentQuery($solrQueryProcessor) {
    
    // set path
    $path = SOLR_SEARCH_PATH . '/' . $solrQueryProcessor->solrQuery; // $_GET['q'] didn't seem to work here
    
    // get date format
    $format = variable_get('islandora_solr_facet_date_format', 'Y');
    
    // set output variable
    $output = '';
    
    // get user provided filter parameters
    $fq = $solrQueryProcessor->internalSolrParams['f'];
    
    // parameters set in url
    $params = $solrQueryProcessor->internalSolrParams;

    
    // Get Query values
    if (!in_array($solrQueryProcessor->solrQuery, $solrQueryProcessor->different_kinds_of_nothing)) {
      // get query value
      $query_value = $solrQueryProcessor->solrQuery;
      
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
      // set rel
      $attr_minus['rel'] = 'nofollow';      
      // set url
      $attr_minus['href'] = url($path_minus, array('query' => $query_minus));
      
      // create link
      // we're not using l() because of active classes: http://drupal.org/node/41595
      $query_list[] = '<a' . drupal_attributes($attr_minus) . '>(-)</a> ' . $query_value;
      
      // add wrap and list
      $output .= '<div class="islandora_solr_search_query_wrap">';
      $output .= theme_item_list($query_list, t('Query'), "ul", array('class' => 'islandora_solr_search_query_list query_list'));
      $output .= '</div>'; // 'islandora_solr_search_query_wrap'
      
    }
    
    
    // Get Filter values
    if (!empty($fq)) {
      
      // set list variables
      $filter_list = array();
      
      // loop over filters
      foreach ($fq as $key => $filter) {

        // check for exclude filter
        if ($filter[0] == '-') {
//          $symbol = '&#2260;';
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
        if (isset($solrQueryProcessor->solrParams['facet.date']) AND in_array(ltrim($filter_split[0], '-'), $solrQueryProcessor->solrParams['facet.date'])) {
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
        // set rel
        $attr_minus['rel'] = 'nofollow';      
        // set url
        $attr_minus['href'] = url($path, array('query' => $query_minus));

        // create link
        // we're not using l() because of active classes: http://drupal.org/node/41595
        $filter_list[] = '<a' . drupal_attributes($attr_minus) . '>(-)</a> ' . $symbol . ' ' . $filter_split[1];
      }
      // return filter list
      $output .= '<div class="islandora_solr_search_filter_wrap">';
      $output .= theme_item_list($filter_list, t("Enabled Filters"), "ul", array('class' => 'islandora_solr_search_filter_list filter_list'));
      $output .= '</div>'; // 'islandora_solr_search_filter_wrap'
    }
    
    return $output;
  }

  /**
   * Function: setBreadcrumbs
   * 
   * Description: sets the drupal breadcrumbs based on the current query and filters. 
   */
  function setBreadcrumbs($solrQueryProcessor) {
    
    // set path
    $path = SOLR_SEARCH_PATH . '/' . $solrQueryProcessor->solrQuery; // $_GET['q'] didn't seem to work here
    
    // get date format
    $format = variable_get('islandora_solr_facet_date_format', 'Y');
    
    // set breadcrumb variable
    $breadcrumb = array();
    
    // get user provided filter parameters
    $fq = $solrQueryProcessor->internalSolrParams['f'];
    
    // parameters set in url
    $params = $solrQueryProcessor->internalSolrParams;
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
        if (isset($solrQueryProcessor->solrParams['facet.date']) AND in_array(ltrim($filter_split[0], '-'), $solrQueryProcessor->solrParams['facet.date'])) {
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
        $breadcrumb[] = '<a' . drupal_attributes($attr) . '>' . $filter_split[1] . '</a>'
              . '<span class="islandora_solr_search_breadcrumb_super"> <a' . drupal_attributes($attr_x) . '>(' . t('x') . ')</a></span>';

      }

      // at this point reverse the breadcrumbs array (only contains filters)
      $breadcrumb = array_reverse($breadcrumb);
    }
    
    // create Query breadcrumb
    if (!in_array($solrQueryProcessor->solrQuery, $solrQueryProcessor->different_kinds_of_nothing)) {
      // get query value
      $query_value = $solrQueryProcessor->solrQuery;
      
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
      $breadcrumb[] = '<a' . drupal_attributes($attr) . '>' . $query_value . '</a>'
            . '<span class="islandora_solr_search_breadcrumb_super"> <a' . drupal_attributes($attr_x) . '>(' . t('x') . ')</a></span>';
    }
    
    $breadcrumb[] = l(t('Home'), '<front>', array('attributes' => array('title' => t('Home'))));
    if (!empty($breadcrumb))
      $breadcrumb = array_reverse($breadcrumb);
    drupal_set_breadcrumb($breadcrumb);
    
  }
  
  /**
   * Function: prepFieldSubstitutions
   *
   * Description: reads configuration values and preps a number of key=>value
   *   arrays for output substitution
   */
  function prepFieldSubstitutions() {
    $rawFacetVals = variable_get("islandora_solr_search_block_facets", 'dc.subject ~ Subject,dc.type ~ Type');
    $this->facetFieldArray = islandora_build_substitution_list($rawFacetVals);

    // populate with date facet readable names
    $rawDateFacetVals = variable_get('islandora_solr_facet_date' , NULL);
    $rawDateFacetVals = islandora_build_substitution_list($rawDateFacetVals);
    if ($rawDateFacetVals != NULL) {
      $this->facetFieldArray = array_merge($this->facetFieldArray, $rawDateFacetVals);
    }

    $rawSearchTerms = variable_get('islandora_solr_searchterms', 'dc.title ~ Title');
    $this->searchFieldArray = islandora_build_substitution_list($rawSearchTerms);

    $rawResultFields = variable_get('islandora_solr_search_result_fields', 'dc.subject ~ Subject,dc.type ~ Type');
    $this->resultFieldArray = islandora_build_substitution_list($rawResultFields);

    $this->allSubsArray = array_merge($this->facetFieldArray, $this->searchFieldArray, $this->resultFieldArray);
  }


  /**
   * Function: displayFacets
   *
   * Description: Displays basic facets based on an apache solr query response,
   *   as contained with the IslandoraSolrQueryProcessor.
   *
   * @global string $base_url
   * @param IslandoraSolrQueryProcessor $solrQueryProcessor
   * @return string
   */
  function displayFacets($solrQueryProcessor) {
    // set variables
    $facet_output = $output = '';

    // shown limit
    $facet_min_count = 1;
    $min_result = variable_get('islandora_solr_search_block_facet_min_count', '2');
    $initial_limit = variable_get('islandora_solr_search_block_facet_shown_limit', '0');
    $facet_limit = variable_get('islandora_solr_search_block_facet_limit', '20');

    // get facet values returned from solr
    $facets = isset($solrQueryProcessor->solrResult->facet_counts->facet_fields) ? $solrQueryProcessor->solrResult->facet_counts->facet_fields : array();
    // object to json
    $facets_json  = json_encode($facets);
    // json  to array
    $facets = json_decode($facets_json, true);
    
    // get date facet values
    $date_facets = isset($solrQueryProcessor->solrResult->facet_counts->facet_dates) ? $solrQueryProcessor->solrResult->facet_counts->facet_dates : array();
    // object to json
    $date_facets_json  = json_encode($date_facets);
    // json to array
    $date_facets_arr = json_decode($date_facets_json, true);
    // clean up facet dates array
    $format = variable_get('islandora_solr_facet_date_format' , 'Y');
    // set variable
    $date_facets = array();
    // loop over each date field
    foreach ($date_facets_arr as $key => $fields) {
      // loop over each field
      $date_facets[$key] = array();
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
          $date_facets[$key][$date]['value'] = format_date(strtotime(trim($date)) + (60 * 60 * 24), 'custom', $format) . ' - ' . format_date(strtotime(trim($date_next)) + (60 * 60 * 24), 'custom', $format);
        }
      }
    }

    // merge facet fields with date facets
    $facets = array_merge($date_facets, $facets);

    // return if no facets are available
    if (empty($facets)) {
      return $output;
    }

    // set show more variable
    $show_more = FALSE;

    // loop over returned facet objects
    foreach ($facets as $facet_key => $facet) {
      $list_items = array();
      $list_type = "ul";
      $list_title = NULL;

      // if the facet contains the minimum amount of values
      if (count($facet) >= $facet_min_count) {
        // set facet count
        $facet_count = 0;

        // render facet values
        foreach ($facet as $value => $data) {
          // add facet
          $this->_addFacets($facet_key, $value, $data, $solrQueryProcessor, $facet_count, $list_items, $min_result);
        }

        // check for minumum returned values again, because some might have been disabled
        if (count($list_items) >= $facet_min_count) {
          
          // opening facet div // @TODO: this should go in theme function
          $facet_output .='<div class="islandora_solr_search_facet">';
          
          // set title
          $list_title = $this->facetFieldArray[$facet_key];

          // set attributes
          $list_attributes = array('class' => 'islandora_solr_search_facet_list facet_list');

          // check if facet value amount is larger than the initial limit
          $split_facet = $facet_count > $initial_limit AND
              $initial_limit < $facet_limit AND
              $initial_limit > $facet_min_count;

          if ($split_facet) {
            $show_more = TRUE; //There exists a split array...
            $list_items_hidden = array_slice($list_items, $initial_limit);
            $list_items = array_slice($list_items, 0, $initial_limit);
          }

          // theme the facet as a list
          $facet_output .= theme('item_list', $list_items, $list_title, $list_type, $list_attributes);

          // render the hidden facets
          if ($split_facet) {
            $facet_output .= '<a href="#" class="shown-toggle">' . t('Show more') . '</a>';
            $facet_output .= '<a href="#" class="shown-toggle hidden">' . t('Show less') . '</a>';
            $facet_output .= '<div class="toggle-wrapper hidden">';
            $facet_output .= theme('item_list', $list_items_hidden, NULL, $list_type, $list_attributes);
            $facet_output .= '</div>';
          }

          $facet_output .='</div>'; //div.islandora_solr_search_facet
        }
      }
    }

    if ($show_more) {
      // if there is at least one occurence of a split array, we add js to the page.
      drupal_add_js(drupal_get_path('module', 'islandora_solr_search') . '/js/islandora_solr_search_shown_limit.js');
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // as we add additional facets, we're repeatedly URL-encoding old facet             //
    // strings. when we double-encode quotation marks they're incomprehensible to solr. //
    // This is a quick workaround:                                                      //
    //////////////////////////////////////////////////////////////////////////////////////
    $facet_output = str_replace('%2B', '%252B', $facet_output);
    if (!empty($facet_output)) {
      $output .='<div class="islandora_solr_search_facet_wrap">';
      $output .= $facet_output;
      $output .= '</div>';
    }

    return $output;
  }

  
  /**
   * Add facet callback
   * @param type $key
   * @param type $name
   * @param type $number
   * @param type $solrQueryProcessor
   * @param int $facet_count
   * @param string $list 
   */
  function _addFacets(&$key, &$value, &$data, &$solrQueryProcessor, &$facet_count, &$list, $min_result) {
    
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
      $filter = $key . ':"' . replaceSlashes($value) . '"'; // PID:"foo"
    }
    
    //FIXME:  This isn't quite right...  It can make sense to facet on 
    //  the same field multiple times, provided it is multi-valued...
    $fq = (array) $solrQueryProcessor->solrParams['fq'];

    // check if item should show up
    // 1. check if the filter isn't active
    // 2. if the results of the facet items equals the total of current returned results, that means it's useless to include or exclude the filter.
    $disable_link = (array_search($filter, $fq) != FALSE) || $data == $solrQueryProcessor->solrResult->response->numFound;

    // if link isn't disabled
    if (!$disable_link) {

      // set variables
      $path = SOLR_SEARCH_PATH . '/' . $solrQueryProcessor->solrQuery; // $_GET['q'] didn't seem to work here

      $query_plus = array();
      $query_minus = array();
      
      // parameters set in url
      $params = $solrQueryProcessor->internalSolrParams;
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
  

  
  function _add_field(&$form, &$i, &$unprocessed_contents, &$types) {
    $type = NULL;
    $temp = preg_split('/:/', $unprocessed_contents, 2);

    if (!isset($type)) {
      if (trim($temp[0]) === '_query_') {
        //We're using a subquery--let's assume dismax
        //get the 'qf'..., dump the rest into 'terms'
        $temp[1] = trim($temp[1]);
        $qf = strpos($temp[1], 'qf=');
        $end = strpos($temp[1], '}');
        $type = substr($temp[1], $qf + 3, $end - $qf - 3);
        $terms = trim(substr($temp[1], $end + 1, -1));
      }
      else {
        $type = $temp[0];
        $terms = trim($temp[1], '()');
      }
    }

    $form['search_type']["type$i"] = array(
      '#title' => '',
      '#type' => 'select',
      '#options' => $types,
      '#default_value' => trim($type)
    );
    $form["fedora_terms$i"] = array(
      '#size' => '24',
      '#type' => 'textfield',
      '#title' => '',
      '#default_value' => stripslashes($terms),
    );
  }

  /**
   * Build solr search form
   * @param type $repeat
   * @param type $pathToSearchTerms
   * @param type $query
   * @return string
   */
  function build_solr_search_form($repeat = NULL, $pathToSearchTerms = NULL, $query = NULL) {
    $types = array();
    $terms = trim(variable_get('islandora_solr_searchterms', 'dc.title ~ Title,dc.subject ~ Subject'));
    $termsArray = preg_split('/[\n]/', $terms);
    foreach ($termsArray as $term) {
      $vals = split('~', $term);
      if (!$vals[1]) {
        $vals[1] = $vals[0];
      }
      $types[trim($vals[0])] = trim($vals[1]);
    }

    $queryArray = NULL;
    if (isset($query)) {
      //A regex like '\\) (OR|AND) \\(' should work, right?  Nope.  Seems that 
      //  preg_split (when given PREG_SPLIT_DELIM_CAPTURE) returns each explicit 
      //  capture group, so for that regex, you'd get only the 'AND' or 'OR'.
      //  If you tried something like '('/(\\) (OR|AND) \\()/')', you'd get the 
      //  'proper' ') AND (' or ') OR (', but also the 'AND' or 'OR' from the 
      //  inner group...  RAGE!...  This turns out to be unused...  Anyway.
      $bool_re = '/(OR|AND)/';
      $queryArray = preg_split($bool_re, $query, 0, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE);
      foreach ($queryArray as $key => $val) {
        if (preg_match($bool_re, $val)) {
          $andOrs[] = trim($val); // that is, just 'OR' or 'AND'
          unset($queryArray[$key]); //remove that from the array
        }
      }
      $queryArray = array_values($queryArray);
    }

    $andOrArray = array(
      'AND' => 'and',
      'OR' => 'or',
    );
    $form = array();

    if (!isset($repeat)) {
      $repeat = variable_get('islandora_solr_search_block_repeat', '3');
    }
    $i = 1;
    $this->_add_field($form, $i, $queryArray[0], $types);

    if ($repeat > 1 && $repeat < 9) { //don't want less then 2 or more then 9
      for ($i = 2; $i < $repeat + 1; $i++) {
        $t = $i - 1;
        $j = $i - 2;
        $andorj = ( isset($andOrs[$j]) ? $andOrs[$j] : 'AND' );

        //$field_and_term = isset($queryArray[$t]) ? explode(':', $queryArray[$t]) : array('', '');
        $form["andor$t"] = array(
          '#title' => '',
          '#type' => 'select',
          '#default_value' => $andorj,
          '#options' => $andOrArray
        );
        $this->_add_field($form, $i, $queryArray[$t], $types);
      }
    }

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('search')
    );
    return $form;
  }

  /**
   * Build simple solr form
   * @global type $islandora_query
   * @return type
   */
  function build_simple_solr_form() {
    // FIXME:  This should actually be checking the settings in the request handler...  Not making assumptions based on name
    $required = FALSE;
    if (variable_get('islandora_solr_search_block_request_handler', '') == 'standard') {
      $required = TRUE;
    }
    global $islandora_query;
    if (!strpos($islandora_query, ':')) {
      $default = $islandora_query;
    }
    // TODO: Fix the above.  The global is no longer valid, and the test is not conclusive.  Should instead test for deftype=dismax.
    $form["islandora_simple_search_query"] = array(
      '#size' => '24%',
      '#type' => 'textfield',
      '#title' => '',
      '#default_value' => $default,
      '#required' => $required,
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('search')
    );
    return $form;
  }

  /**
   * Theme solr search form
   * @param type $form
   * @return type
   */
  function theme_solr_search_form($form) {
    if (!isset($repeat)) {
      $repeat = variable_get('islandora_solr_search_block_repeat', '3');
    }

    $output = drupal_render($form['search_type']['type1']);
    $output .= drupal_render($form['fedora_terms1']);
    $output .= drupal_render($form['andor1']) . drupal_render($form['search_type']['type2']);
    $output .= drupal_render($form['fedora_terms2']);
    if ($repeat > 2 && $repeat < 9) {
      for ($i = 3; $i < $repeat + 1; $i++) {
        $t = $i - 1;
        $output .= drupal_render($form["andor$t"]) . drupal_render($form['search_type']["type$i"]);
        $output .= drupal_render($form["fedora_terms$i"]);
      }
    }
    $output .= drupal_render($form['submit']);
    $output .= drupal_render($form);
    return $output;
  }

  /**
   * Merges snippet fields with the rest of the solr results
   * @param Apache Solr Object $results
   * @return array
   */
  function prep_results($results) {
    $rawResponse = $results->getRawResponse();
    $responseArray = json_decode($rawResponse, TRUE);
    $highlights = $responseArray['highlighting'];
    $docs = $responseArray['response']['docs'];

    if (!is_array($highlights)) {
       return $docs;
    }

   // if highights exist, inetegrate them back into the doc array
    $label = variable_get("islandora_solr_snippet_field", "");
    if ($label) {
      $label_parts = explode('~', $label);
      $field_label = trim($label_parts[1]);
      $field_text = trim($label_parts[0]);
    }
    $this->resultFieldArray[$field_text]= $field_label;
    $this->allSubsArray[$field_text]= $field_label;
    foreach ($docs as &$doc) {
      $pid = $doc['PID'];
      unset($doc[$field_text]);
      $snippet = $highlights[$doc['PID']];
      foreach ($snippet as $key => $value) {
        $doc[$field_text] = implode('<br />', $value);
      }
    }
    return $docs;
  }

}

