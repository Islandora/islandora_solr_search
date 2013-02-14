<?php

/**
 * @file
 * Contains all blocks related functionality
 */

/**
 * Implements hook_islandora_solr_query_blocks().
 */
function islandora_solr_islandora_solr_query_blocks() {
  return array(
    // 'machine_name' => array(
    //   'name' => t('Human Readable Name'),
    //   'module' => 'module_name',
    //   'file' => 'FileName.inc',
    //   'class' => 'ClassName',
    //   'function' => 'method_name',
    //   'form' => 'form_function_name',
    //  ),
    //
    // Note: As in the examples below, it is valid to specify
    // *either* a class and method *or* a form.  The latter implies no
    // class needs to be instantiated.
    //

    'advanced' => array(
      'name' => t('Islandora advanced search'),
      'module' => 'islandora_solr',
      'file' => 'includes/blocks.inc',
      'class' => NULL,
      'function' => NULL,
      'form' => 'islandora_solr_advanced_search_form',
    ),
    'simple' => array(
      'name' => t('Islandora simple search'),
      'module' => 'islandora_solr',
      'file' => 'includes/blocks.inc',
      'class' => NULL,
      'function' => NULL,
      'form' => 'islandora_solr_simple_search_form',
    ),
    'basic_facets' => array(
      'name' => t('Islandora facets'),
      'module' => 'islandora_solr',
      'file' => 'includes/results.inc',
      'class' => 'IslandoraSolrResults',
      'function' => 'displayFacets',
      'form' => NULL,
    ),
    'current_query' => array(
      'name' => t('Islandora query'),
      'module' => 'islandora_solr',
      'file' => 'includes/results.inc',
      'class' => 'IslandoraSolrResults',
      'function' => 'currentQuery',
      'form' => NULL,
    ),
    'display_switch' => array(
      'name' => t('Islandora displays'),
      'module' => 'islandora_solr',
      'file' => 'includes/blocks.inc',
      'class' => NULL,
      'function' => 'islandora_solr_display',
      'form' => NULL,
    ),
    'sort' => array(
      'name' => t('Islandora sort'),
      'module' => 'islandora_solr',
      'file' => 'includes/blocks.inc',
      'class' => NULL,
      'function' => 'islandora_solr_sort',
      'form' => NULL,
    ),
  );
}


/**
 * Implements hook_block_info().
 */
function islandora_solr_block_info() {
  // set variable
  $blocks = array();

  // return a list of block descriptions for block administration
  $solr_blocks = module_invoke_all("islandora_solr_query_blocks");
  foreach ($solr_blocks as $name => $block ) {
    $blocks[$name] = array(
      'info' => $block['name'],
      'cache' => DRUPAL_NO_CACHE,
    );
  }
  return $blocks;
}


/**
 * Implements hook_block_view().
 *
 * @global type $_islandora_solr_queryclass
 *   the IslandoraSolrQueryProcessor object which includes the current query
 *   settings and the raw Solr results.
 * @param type $block_name
 *   delta parameter for blocks. This is the unique block name.
 * @return type
 *   returns an array containing the data to display in the block
 */
function islandora_solr_block_view($block_name = '') {
  // set variable
  $block = array();

  // get global class
  global $_islandora_solr_queryclass;

  // get solr blocks
  $solr_blocks = module_invoke_all("islandora_solr_query_blocks");

  // if the block that is called is defined
  if (!empty($solr_blocks[$block_name]) && user_access('search islandora solr')) {

    // set block title
    $block['subject'] = $solr_blocks[$block_name]['name'];

    // Include the file from which the block originates.
    require_once( drupal_get_path('module', $solr_blocks[$block_name]['module']) . '/' . $solr_blocks[$block_name]['file'] );

    // if a class is present, instantiate it and proceed.
    // The variable $_islandora_solr_queryclass (the IslandoraSolrQueryProcessor, containing
    // the Solr search result), is fed as an argument.
    if (!empty($solr_blocks[$block_name]['class']) && !empty($solr_blocks[$block_name]['function'])) {
      // check if we're on a search results page
      if (islandora_solr_results_page($_islandora_solr_queryclass)) {
        $display_class = new $solr_blocks[$block_name]['class']();
        $block_function = $solr_blocks[$block_name]['function'];
        if (method_exists($display_class, $block_function)) {
          // set content
          $block['content'] = $display_class->$block_function($_islandora_solr_queryclass);
        }
      }
      // otherwise, simply load the form.
    }
    elseif (!empty($solr_blocks[$block_name]['form'])) {
      // set content
      $block['content'] = drupal_get_form($solr_blocks[$block_name]['form']);
    }
    // else if only a function is given (no class, no form)
    elseif (!empty($solr_blocks[$block_name]['function'])) {
      $function = $solr_blocks[$block_name]['function'];
      // set content
      $block['content'] = $function();
    }
  }
  return $block;
}



/**
 * Islandora Solr simple search form
 *
 * @param type $form
 * @param type $form_state
 * @return string
 */
function islandora_solr_simple_search_form($form, &$form_state) {

  $form['simple'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array(
        'container-inline')
      )
    );
  $form['simple']["islandora_simple_search_query"] = array(
    '#size' => '15',
    '#type' => 'textfield',
    '#title' => '',
    '#default_value' => '', // @TODO: should this be the searched value?
  );
  $form['simple']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('search')
  );
  return $form;
}


/**
 * Islandora Solr simple search form submit
 *
 * @param type $form
 * @param array $form_state
 */
function islandora_solr_simple_search_form_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $search_string = $form_state['values']['islandora_simple_search_query'];
  $search_string = str_replace('/', '~slsh~', $search_string); //replace the slash so url doesn't break

  // set query
  $query = array('type' => 'dismax');

  drupal_goto('islandora/search/' . $search_string, array('query' => $query));
}



/**
 * Islandora Solr advanced search block form
 *
 * @global type $_islandora_solr_queryclass
 *   the IslandoraSolrQueryProcessor object which includes the current query
 *   settings and the raw Solr results.
 * @param type $form
 * @param array $form_state
 * @return string
 */
function islandora_solr_advanced_search_form($form, &$form_state) {
  global $_islandora_solr_queryclass;

  // include common.inc
  module_load_include('inc', 'islandora_solr', 'includes/common');

  // check form states
  // 1. form update using ajax
  // 2. populate with current query on search results page
  // 3. anywhere else: empty form

  // 1. form update using ajax
  if (isset($form_state['triggering_element'])) {

    // unset terms from input array
    // http://drupal.stackexchange.com/questions/14855/how-do-i-dynamically-fill-a-textfield-with-ajax/16576#16576
    unset($form_state['input']['terms']);
    // check for add
    if ($form_state['triggering_element']['#value'] == '+') {
      $form_state['values']['terms'][] = array();
    }
    // check for remove
    elseif ($form_state['triggering_element']['#value'] == '-') {
      $field = $form_state['triggering_element']['#field'];
      array_splice($form_state['values']['terms'], $field, 1);
    }
    // set values
    $values = $form_state['values'];
  }
  // 2. populate with current query on search results page
  elseif (islandora_solr_results_page($_islandora_solr_queryclass) == TRUE && !isset($_islandora_solr_queryclass->internalSolrParams['type'])) {

    // get current query
    $query = $_islandora_solr_queryclass->solrQuery;
    // set value variable
    $values['terms'] = array();

    // explode on space
    $query_explode = explode(' ', $query);

    // break up the solr query to populate the advanced search form
    $i = 0;
    foreach ($query_explode as $key => $value) {
      $term = array();

      // check for first colon to split the string
      if (strpos($value, ':') != FALSE) {
        // split the filter into field and value
        $value_split = explode(':', $value, 2);
        // set field
        $values['terms'][$i]['field'] = $value_split[0];

        // second part of the split is the query value (or first part of it)
        $value_split[1] = str_replace(array('(', ')'), '', $value_split[1]);
        // add search string
        $values['terms'][$i]['search'] = $value_split[1];

      }
      // if the string does not include a colon or AND/OR/NOT, then it is a
      // part of the query value
      elseif (!preg_match('/(AND|OR|NOT)/', $value, $matches)) {
        // trim brackets
        $value = str_replace(array('(', ')'), '', $value);

        if (isset($values['terms'][$i]['search'])) {
          // append to search string
          $values['terms'][$i]['search'] .= ' ' . $value;
        }
        else {
          // search field is not set, so create new search value
          $values['terms'][$i]['search'] = $value;
        }
      }
      // if it matches AND/OR/NOT, then we have the boolean operator
      else {
        // set boolean operator
        $values['terms'][$i]['boolean'] = $value;

        // increment to next field
        $i++;
      }
    }
  }
  // 3. anywhere else: empty form
  else {
    //Need at least one term to draw the search box.
    $values = array(
      'terms' => array(''),
    );
  }
  $terms = array(
    '#type' => 'markup',
    '#prefix' => '<div id="islandora-solr-advanced-terms">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
  );
  // loop over each term
  foreach ($values['terms'] as $i => $value) {
    $term = array(
      '#tree' => TRUE,
      '#prefix' => '<div>',
      '#suffix' => '</div>',
    );
    $term['field'] = array(
      '#title' => t('Field'),
      '#type' => 'select',
      '#default_value' => isset($value['field']) ? $value['field'] : 'dc.title',
      '#options' => islandora_solr_get_fields('search_fields'),
    );
    $term['search'] = array(
      '#title' => t('Search terms'),
      '#type' => 'textfield',
      '#size' => 20,
      '#default_value' => isset($value['search']) ? $value['search'] : '',
    );
    $term['hidden_submit'] = array(// Used for when the user presses enter on the search field.
      '#type' => 'submit',
      '#value' => t('Search'),
      '#attributes' => array('style' => 'visibility:hidden;position:fixed;top:-1000px;right:-1000px;')
    );
    $term['add'] = array(
      '#type' => 'button',
      '#value' => '+',
      '#attributes' => array('title' => t('Add field')),
      '#name' => 'add-field-' . $i,
      '#ajax' => array(
        'callback' => '_islandora_solr_advanced_search_terms',
        'wrapper' => 'islandora-solr-advanced-terms',
        'method' => 'replace',
        'effect' => 'fade',
        'progress' => array('type' => 'none'),
      )
    );
    if (count($values['terms']) > 1) {
      $term['remove'] = array(
        '#type' => 'button',
        '#field' => $i,
        '#value' => '-',
        '#attributes' => array('title' => t('Remove field')),
        '#name' => 'remove-field-' . $i,
        '#ajax' => array(
          'callback' => '_islandora_solr_advanced_search_terms',
          'wrapper' => 'islandora-solr-advanced-terms',
          'method' => 'replace',
          'effect' => 'fade',
          'progress' => array('type' => 'none'),
        )
      );
      if ((variable_get('islandora_solr_search_boolean', 'user') == 'user') && ((count($values['terms']) -1) != $i)) {
        $term['boolean'] = array(
          '#type' => 'select',
          '#prefix' => '<div>',
          '#suffix' => '</div>',
          '#default_value' => isset($value['boolean']) ? $value['boolean'] : 'AND',
          '#options' => array(
            'AND' => 'AND',
            'OR' => 'OR',
            'NOT' => 'NOT'
          )
        );
      }
    }
    $terms[] = $term;
  }
  // set form
  $form = array();
  // add terms
  $form['terms'] = $terms;
  // add controls
  $form['controls'] = array(
    '#type' => 'markup',
    '#prefix' => '<div class="islandora-solr-advanced-controls">',
    '#suffix' => '</div>',
  );
  $form['controls']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Search')
  );
  return $form;
}



/**
 * AJAX callback for advanced search form
 *
 * @param type $form
 * @param type $form_state
 * @return type
 *
 * @see islandora_solr_advanced_search_form()
 */
function _islandora_solr_advanced_search_terms($form, &$form_state) {
  return $form['terms'];
}


/**
 * Islandora Solr advanced search form submit
 *
 * @param type $form
 * @param type $form_state
 *
 * @see islandora_solr_advanced_search_form()
 */
function islandora_solr_advanced_search_form_submit($form, &$form_state) {

  // Collect query values
  $query_array = array();
  foreach ($form_state['values']['terms'] as $term) {
    // set field
    $field = $term['field'];
    // set search value
    $search = trim($term['search']);
    //replace the slash so url doesn't break
    $search = str_replace('/', '~slsh~', $search);
    // set boolean
    $boolean = (isset($term['boolean'])) ? $term['boolean'] : variable_get('islandora_solr_search_boolean', 'user');

    // add query
    if (!empty($search)) {
      $query_array[] = array(
        'search' => "$field:($search)",
        'boolean' => $boolean
      );
    }
  }

  // create query
  $query = '';
  $i = 0;
  foreach ($query_array as $term) {
    $query .= $term['search'];
    if (count($query_array) -1 != $i) {
      $query .= ' ' . $term['boolean'] . ' ';
    }
    $i++;
  }

  // check if query is empty
  if (empty($query)) {
    $query = '*:*';
  }

  // navigate to results page
  drupal_goto('islandora/search/' . $query);
}




/**
 * Provides list to change primary displays. Only populated with displays
 * that are enabled through the admin form.
 *
 * @global type $_islandora_solr_queryclass
 *   the IslandoraSolrQueryProcessor object which includes the current query
 *   settings and the raw Solr results.
 * @return
 *   a rendered list containing enabled primary display links
 */
function islandora_solr_display() {
  global $_islandora_solr_queryclass;

  // set variables
  $output = '';

  // check if the page is dealing with search results
  if (islandora_solr_results_page($_islandora_solr_queryclass)) {

    // parameters set in url
    $params = isset($_islandora_solr_queryclass->internalSolrParams) ? $_islandora_solr_queryclass->internalSolrParams : array();
    // set path
    $path = SOLR_SEARCH_PATH . '/' . replace_slashes($_islandora_solr_queryclass->solrQuery);

    // get profiles
    $profiles = module_invoke_all("islandora_solr_primary_display");
    // get the table settings
    $primary_display_array = variable_get('islandora_solr_primary_display_table', array());

    if (!empty($primary_display_array)) {

      $weight = $primary_display_array['weight'];
      $enabled = $primary_display_array['enabled'];
      $default = $primary_display_array['default'];

      // sort by weight
      array_multisort(array_values($weight), SORT_ASC, array_keys($weight), SORT_ASC, $weight);

      // set variables
      $list_items = array();

      foreach ($weight as $key => $value) {
        if ($enabled[$key] !== 0) {

          // current display
          if (isset($params['display'])) {
            $current_display = $params['display'];
          }
          else {
            $current_display = $default;
          }

          // title
          $display_name = $profiles[$key]['name'];

          // description
          $display_description = isset($profiles[$key]['description']) ? $profiles[$key]['description'] : NULL;

          // query
          $query = array_merge($params, array('display' => $key));

          // set attributes
          $attr = array();
          // set class
          $attr['class'] = 'display-' . drupal_strtolower( preg_replace('/[^A-Za-z0-9]/', '-', $key));
          if ($key == $current_display) {
            $attr['class'] .= ' active';
          }
          // nofollow
          $attr['rel'] = 'nofollow';
          // title
          $attr['title'] = $display_description;
          // url
          $attr['href'] = url($path, array('query' => $query));

          // create link
          // we're not using l() because of active classes: http://drupal.org/node/41595
          $item = '<a' . drupal_attributes($attr) . '>' . check_plain($display_name) . '</a>';

          // add link to list
          $list_items[] = $item;
        }
      }
      // theme item list (if more than one displays are available)
      if (count($list_items) > 1) {
        $output = theme('item_list', array('items' => $list_items, 'title' => NULL, 'type' => 'ul', 'attributes' => array('class' => 'islandora-solr-display')));
      }
    }
  }
  return $output;
}


/**
 * Provides a list to change the sorting on a current search query.
 *
 * @global type $_islandora_solr_queryclass
 *   the IslandoraSolrQueryProcessor object which includes the current query
 *   settings and the raw Solr results.
 * @return
 *   a rendered list containing Solr sort links
 *
 */
function islandora_solr_sort() {
  global $_islandora_solr_queryclass;

  // set variables
  $output = '';

  // check if the page is dealing with search results
  if (islandora_solr_results_page($_islandora_solr_queryclass)) {

    // parameters set in url
    $params = isset($_islandora_solr_queryclass->internalSolrParams) ? $_islandora_solr_queryclass->internalSolrParams : array();
    // set path
    $path = SOLR_SEARCH_PATH . '/' . replace_slashes($_islandora_solr_queryclass->solrQuery);

    // get sort terms
    $sort_terms = islandora_solr_get_fields('sort_fields');

    // get current sort (check in the actual parameters used for the query, because modules might have altered it)
    if (isset($_islandora_solr_queryclass->solrParams['sort'])) {
      $sort_explode = explode(' ', $_islandora_solr_queryclass->solrParams['sort']);
      $current_sort_term = $sort_explode[0];
      $current_sort_order = isset($sort_explode[1]) ? $sort_explode[1] : NULL;
    }
    else {
      // if no parameters are given, internally solr defaults to 'score desc'
      // http://wiki.apache.org/solr/CommonQueryParameters#sort
      $current_sort_term = 'score';
      $current_sort_order = 'desc';
    }

    // set variables
    $list_items = array();

    // loop over sort terms
    foreach ($sort_terms as $term => $label) {

      // set indicator variable
      $indicator = '';

      // create query and indicator arrow
      if ($term == $current_sort_term) {
        if (isset($current_sort_order) && $current_sort_order == 'asc') {
          // set order
          $order = 'desc';
          // create indicator
          $indicator = theme('tablesort_indicator', array('style' => 'desc'));
        }
        else {
          // set order
          $order = 'asc';
          // create indicator
          $indicator = theme('tablesort_indicator', array('style' => 'asc'));
        }
      }
      // the term is score or a date, we want descending order by default
      elseif ($term == 'score' || stripos($term, 'date')) {
        // set order
        $order = 'desc';
      }
      else {
        // set order
        $order = 'asc';
      }

      // set sort parameter
      $sort_param = $term . ' ' . $order;

      // set query
      $query = array_merge($params, array('sort' => $sort_param));

      // set attributes
      $attr = array();
      // set class
      if ($term == $current_sort_term) {
        $attr['class'] = 'active';
      }
      // nofollow
      $attr['rel'] = 'nofollow';
      // title
      $attr['title'] = $label;
      // url
      $attr['href'] = url($path, array('query' => $query));

      // create link
      // we're not using l() because of active classes: http://drupal.org/node/41595
      $item = '<a' . drupal_attributes($attr) . '>' . $label . ' ' . $indicator . '</a>';

      // add link to list
      $list_items[] = $item;
    }

    // theme item list
    if (!empty($list_items)) {
      $output = theme('item_list', array('items' => $list_items, 'title' => NULL, 'type' => 'ul', 'attributes' => array('class' => 'islandora-solr-sort')));
    }
  }

  return $output;
}