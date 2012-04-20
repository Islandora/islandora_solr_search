<?php

/**
 * @file
 * Contains methods to search solr and display results.  depends on Apache_Solr_Php client.
 */

/**
 * Islandora Solr Results
 */
class IslandoraSolrResults {

  static $facetSeparator = '~'; //used to separate facets in url
  static $slashReplacement = '~slsh~'; // a pattern used to replace / in url's the slash breaks drupal clean url's
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
    if (is_null($title)) { //Needed to move this, as default parameters cannot be function calls
      $title = t("Search Results");
    }
    $apacheSolrResult = $solrQueryProcessor->solrResult;
    $total = (int) $apacheSolrResult->response->numFound;

    $end = min(($solrQueryProcessor->solrLimit + $solrQueryProcessor->solrStart), $total);

    // Initialize drupal-style pager
    islandora_solr_search_pager_init($total, $solrQueryProcessor->solrLimit);

    $output .= $this->addSecondaries();

    $output .= '<h2>' . $title . '</h2>';
    $output .= '<div id="islandora_solr_result_count">(' . ($solrQueryProcessor->solrStart + ($total > 0 ? 1 : 0)) . ' - ' . $end . ' of ' . $total . ')</div>';
    $output .= '<div class="content">';

//    drupal_set_message(print_r($secondary_display_profiles,1));
    // Render Drupal pager (top of search results)
    $output .= theme('pager', NULL, $solrQueryProcessor->solrLimit, 0, NULL, 5);

    /*
     * Then, here, we need to build links to the current query under the secondary query display
     * profile for each such profile which uses the hook.
     *
     * The link should basically be $theCurrentURL with &proile=<profile_name> tacked on.
     *
     * Also, perhaps there's no difference between primary and secondary profiles?
     */
    $output .= $this->printResults($apacheSolrResult);
    if (variable_get('islandora_solr_search_debug_mode', FALSE)) {
      $output .= $this->printDebugOutput($solrResult);
    }

    // Render Drupal pager (bottom of search results)
    $output .= theme('pager', NULL, $solrQueryProcessor->solrLimit, 0, NULL, 5);


    $output .= "</div>"; // class = "content"
    return $output;
  }

  function addSecondaries() {
    $output = '';
    // Get secondary display profiles
    $secondary_display_profiles = module_invoke_all('islandora_solr_secondary_display');
    // How about this for an expected return?
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
    // check if
    // if the islandora_solr_search admin form has never been saved, then
    // the variable 'islandora_solr_secondary_display' won't be set yet.
    $secondary_array = variable_get('islandora_solr_secondary_display', array());

    // count number of elements in array
    $array_count = count($secondary_array);
    // count number of disabled displays
    $i = 0;
    foreach ($secondary_array as $key => $value) {
      if ($value === 0) {
        $i++;
      }
    }

    // if number of elements is the same as the number of disabled displays,
    // this means that there are no displays enabled.
    if ($array_count != $i) {

      $output .= "<div id='islandora_solr_secondary_display_profiles'>";

      foreach ($secondary_array as $name => $status) {
        if ($status === $name) {

          $description = $secondary_display_profiles[$name]['description'];
          $logo = $secondary_display_profiles[$name]['logo'];

          $attributes = array('title' => $description);
          $query = array();
          $query[] = drupal_query_string_encode(array('solr_profile' => $name));
          $output .= l($logo, $_GET['q'], array('attributes' => $attributes, 'html' => TRUE, 'query' => count($query) ? implode('&', $query) : NULL));
        }
      }

      $output .= "</div>";
    }
    return $output;
  }

  /**
   * Function: printResults
   *
   * Description: translates a solr query result into a basic in-browser search result.
   *
   * @param Apache_Solr_Object $results
   */
  function printResults($results) {
    // Check for empty resultset
    if ((int) $results->response->numFound === 0) {
      $output .= "<p>Sorry, but your search returned no results.</p>";
      return $output;
    }
    $docs = $this->prep_results($results);

    $items = array();
    $type = "ol";
    $title = NULL;
    $recordStart = $results->response->start;
    $limitResults = variable_get('islandora_solr_search_limit_result_fields', 0);

    global $base_url;
    if (empty($results)) {
      return "no results";
    }

    foreach ($docs as $doc) {
      $rows = array();
      $row = 0;
      $snippet = NULL;
      foreach ($doc as $field => $value) {

        if ($limitResults && empty($this->resultFieldArray[$field])) {
          continue;
        }

        $translated_field_name = isset($this->allSubsArray[$field]) ? $this->allSubsArray[$field] : $field;
        $rows[$row][] = array(
          'data' => $translated_field_name,
          'header' => TRUE,
        );
        if (is_array($value)) {
          $value = implode(", ", $value);
        }

        if ($field == 'PID') {
          $pid = $value;
          $l = l($value, 'fedora/repository/' . htmlspecialchars($value, ENT_QUOTES, 'utf-8'));
          $rows[$row][] = $l;
        }
        else {
          $rows[$row][] = $value;
        }
        $row++;
      }
      if ($snippet) {
        $rows[$row][] = array(
          'data' => 'Full Text',
          'header' => TRUE,
        );
        $rows[$row][] = $snippet[0];
      }
      $items[] = theme_table(NULL, $rows, array('class' => 'islandora_solr_search_results_object'));
    }

    $output .= theme_item_list($items, $title, $type, array('class' => 'stuff', 'start' => ($recordStart + 1)));
    return $output;
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
    //dsm($solrQueryProcessor);
    global $base_url;
    $output = '';
    $islandora_fq = restoreSlashes($solrQueryProcessor->solrFilters);
    $islandora_facets = isset($solrQueryProcessor->solrResult->facet_counts->facet_fields) ?
        $solrQueryProcessor->solrResult->facet_counts->facet_fields :
        NULL;
    //dsm($islandora_facets);
    // set display variable
    $display = $solrQueryProcessor->display;

    if ($display == variable_get('islandora_solr_primary_display', 'default')) {
      $display_param = '';
    }
    else {
      $display_param = $display;
    }


    if (drupal_strlen(trim($solrQueryProcessor->solrQuery))) {

      // Variables to build a drupal-ish unordered list
      $query_list_title = t('Query');
      $query_list_items = array();
      $query_list_type = "ul";
      $query_list_attributes = array('class' => 'islandora_solr_search_query_list query_list');

      // OK, so it's a list of one.
      // In the future, we could reasonably break the query on boolean operators
      // and allow one part to be removed at a time.
      $query = " ";
      $fq = replaceSlashes($islandora_fq);
      $defType = $solrQueryProcessor->solrDefType;
      $repr = '-'; //Again, don't explicitly specify an image...
      $link = "islandora/solr/search/$query/$fq/$defType/$display_param";
      $query_list_items[] .= l($repr, $link,
              array(
                'attributes' => array(
                  'class' => 'islandora_solr_add_remove_link'))
          ) . ' ' . $this->getSubstitutedFields(stripslashes($solrQueryProcessor->solrQuery), FIELD_SEARCH);

      $output .= '<div class="islandora_solr_search_query_wrap">';

      $output .= theme_item_list($query_list_items, $query_list_title, $query_list_type, $query_list_attributes);
      $output .= '</div>'; // 'islandora_solr_search_query_wrap'
    }

    //////////////////////////////////////////////////////////////////////////////
    // Here we will build links to remove currently enforced facet filters ...  //
    //////////////////////////////////////////////////////////////////////////////
    $filters = empty($islandora_fq) ? NULL : explode(IslandoraSolrResults::$facetSeparator, $islandora_fq);

    if (!empty($filters) && $islandora_fq != '-') {
//      $filter_output = NULL;
      // Variables to build a drupal-ish unordered list, which will be rendered
      // a little bit farther down
      $filter_list_title = t("Enabled Filters");
      $filter_list_items = array();
      $filter_list_type = "ul";
      $filter_list_attributes = array('class' => 'islandora_solr_search_filter_list filter_list');

      foreach ($filters as $key => $filter) {
        $edit_filters = $filters;
        unset($edit_filters[$key]);
        $filter_disable = implode(IslandoraSolrResults::$facetSeparator, $edit_filters);
        if (empty($filter_disable)) {
          $filter_disable = '-';
        }
        $filter_name_and_value = explode(":", $filter, 2);
        $filter_name = $filter_name_and_value[0];
        $filter_value = $filter_name_and_value[1];
        if (strncmp($filter_name, '-', 1))
          $exclude = FALSE;
        else {
          $exclude = TRUE;
          $filter_name = drupal_substr($filter_name, 1);
        }

        $query = replaceSlashes($solrQueryProcessor->solrQuery);
        $fq = replaceSlashes($filter_disable);
        $defType = $solrQueryProcessor->solrDefType;
        $repr = '-'; //I know that Kris has complained about the remove image before...  This should allow it to be changed.
        $link = "islandora/solr/search/$query/$fq/$defType/$display_param";
        $filter_list_items[] = l($repr, $link,
                array(
                  'attributes' => array(
                    'class' => 'islandora_solr_add_remove_link'))
            ) . ' ' . $this->facetFieldArray[$filter_name] . ($exclude ? ' != ' : ' = ') . $filter_value;
      }
    }

    if (!empty($filter_list_items)) {
//      $filter_output = str_replace('%2B','%252B',$filter_output);
      $output .= '<div class="islandora_solr_search_filter_wrap">';
      $output .= theme_item_list($filter_list_items, $filter_list_title, $filter_list_type, $filter_list_attributes);
      $output .= '</div>'; // 'islandora_solr_search_filter_wrap'
    }

    return $output;
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

    $rawSearchTerms = variable_get('islandora_solr_searchterms', 'dc.title ~ Title');
    $this->searchFieldArray = islandora_build_substitution_list($rawSearchTerms);

    $rawResultFields = variable_get('islandora_solr_search_result_fields', 'dc.subject ~ Subject,dc.type ~ Type');
    $this->resultFieldArray = islandora_build_substitution_list($rawResultFields);

    $this->allSubsArray = array_merge($this->facetFieldArray, $this->searchFieldArray, $this->resultFieldArray);
  }

  function _addFacets(&$key, &$name, &$number, &$solrQueryProcessor, &$facet_count, &$list, $display_param) {
    $islandora_fq = $solrQueryProcessor->solrFilters;
    //FIXME:  This isn't quite right...  It can make sense to facet on 
    //  the same field multiple times, provided it is multi-valued...
    $fq = (array) $solrQueryProcessor->solrParams['fq'];
    //FIXME:  This instance of 't' should be changed to format_string for Drupal 7 (not really translateable)
    $filter = t('!key:"!name"', array(
          '!key' => $key,
          '!name' => replaceSlashes($name)
        ));

    $disable_link = (array_search($filter, $fq) !== FALSE) || $number == $solrQueryProcessor->solrResult->response->numFound;

    //$disable_link = strpos($islandora_fq, $key . ':"' .  . '"') !== FALSE; //we don't want a link for this facet as we already used it
    if ($islandora_fq && $islandora_fq != '-') {//there are existing facets in the query
      if ($disable_link) {
        //don't show link to this facet but include a link to remove it
      }
      else {
        //FIXME:  This instance of 't' should be changed to format_string for Drupal 7 (not really translateable)
        $filter_include = t('!filter!sep!remaining', array(
              '!filter' => $filter,
              '!sep' => IslandoraSolrResults::$facetSeparator,
              '!remaining' => $islandora_fq
            ));
        $filter_exclude = "-$filter_include";
      }
    }
    else { //no existing facets chosen
      $filter_include = $filter;
      $filter_exclude = "-$filter_include";
    }

    $filter_include = replaceSlashes($filter_include); //replace the slash so url does not break
    $filter_exclude = replaceSlashes($filter_exclude); //replace the slash so url does not break
    if ($disable_link) {
      // we don't want to create a link, because we're already filtering on this value.
      // Links to remove enabled facet filters are created down below.
    }
    else {//normal link
      $dt = $solrQueryProcessor->solrDefType;
      $q = $solrQueryProcessor->solrQuery;

      $lplus = "islandora/solr/search/$q/$filter_include/$dt/$display_param";
      $lminus = "islandora/solr/search/$q/$filter_exclude/$dt/$display_param";
      $text = $name;
      $attrs = array();

      global $current_display;
      $display = isset($_GET['display']) ? $current_display : '';
      if ($display) {
        $attrs['query'] = array('display' => $display);
      }

      $list[] = l($text, $lplus, $attrs) . ' (' . $number . ')' .
          '<span class="plusminus">' .
          l('+', $lplus, array_merge($attrs, array('attributes' => array('class' => 'plus'))))
          . ' ' .
          l('-', $lminus, array_merge($attrs, array('attributes' => array('class' => 'minus'))))
          . '</span>';
      $facet_count += 1;
    }
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
    global $base_url;
    $default_display = variable_get('islandora_solr_primary_display', 'default');
    $facet_output = $output = '';
    $islandora_facets = isset($solrQueryProcessor->solrResult->facet_counts->facet_fields) ? $solrQueryProcessor->solrResult->facet_counts->facet_fields : array();

    if (empty($islandora_facets)) {
      return $output; //no facets to show
    }

    // set show more variable
    $show_more = FALSE;

    // set display
    $display = $solrQueryProcessor->display;

    if ($display == $default_display) {
      $display_param = '';
    }
    else {
      $display_param = $display;
    }

    foreach ($islandora_facets as $key => $field) {
      $list_items = array();
      $list_type = "ul";
      $list_title = NULL;

      $test = get_object_vars($field); //get the number of fields if there aren't any don't show the key
      if (count($test) > 0) {
        $facet_count = 0;
        unset($normal_facet_output);

        foreach ($field as $name => $number) {
          $this->_addFacets($key, $name, $number, $solrQueryProcessor, $facet_count, $list_items, $display_param);
        }

        if (count($list_items) > 0) {
          $facet_output .='<div class="islandora_solr_search_facet">';

          // shown limit
          $shown_limit = variable_get('islandora_solr_search_block_facet_shown_limit', 0);
          $facetlimit = variable_get('islandora_solr_search_block_facet_limit', '12');
          $facetMinCount = variable_get('islandora_solr_search_block_facet_min_count', '2');

          $list_title = $this->facetFieldArray[$key];
          $list_type = "ul";
          $list_attributes = array(
            'class' => implode(' ', array(
              'islandora_solr_search_facet_list',
              'facet_list'))
          );

          // split the array if shown_limit is between the min and max limits and if the total is larger than the shown_limit.
          $split_facet = $facet_count > $shown_limit AND
              $shown_limit < $facetlimit AND
              $shown_limit > $facetMinCount;

          if ($split_facet) {
            $show_more = TRUE; //There exists a split array...
            $list_items_hidden = array_slice($list_items, $shown_limit);
            $list_items = array_slice($list_items, 0, $shown_limit);
          }

          $facet_output .= theme_item_list($list_items, $list_title, $list_type, $list_attributes);

          // render the hidden facets
          if ($split_facet) {
            $facet_output .= '<a href="#" class="shown-toggle">' . t('Show more') . '</a>';
            $facet_output .= '<a href="#" class="shown-toggle hidden">' . t('Show less') . '</a>';
            $facet_output .= '<div class="toggle-wrapper hidden">';
            $facet_output .= theme_item_list($list_items_hidden, NULL, $list_type, $list_attributes);
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

