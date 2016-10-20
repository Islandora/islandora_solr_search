<?php

/**
 * @file
 * This file contains functions to create breadcrumbs using Solr.
 */

/**
 * Implements callback_islandora_breadcrumbs_backends.
 */
function islandora_solr_islandora_breadcrumbs_backends_callable($object) {
  $breadcrumbs = islandora_solr_get_breadcrumbs_recursive($object->id);
  array_pop($breadcrumbs);
  return $breadcrumbs;
}

/**
 * Loads a map of collection hierarchy.
 *
 * @return array
 *   array(
 *     this_object_pid => array(
 *       'parent' => parent_pid,
 *       'label' => this_object_label,
 *       ),
 *   );
 */
function islandora_solr_load_crumbs() {
  if (user_is_anonymous()) {
    $cache_roles = 'anonymous';
  }
  else {
    global $user;
    $roles = $user->roles;
    // Sort so we keep ordering consistent to reuse cache.
    asort($roles);
    $cache_roles = implode('_', $roles);
  }

  $crumb_cache = &drupal_static(__FUNCTION__);

  $breadcrumbs = array();

  if (!isset($crumb_cache)) {
    $cache = cache_get(ISLANDORA_SOLR_BREADCRUMB_CACHE_NAME, 'cache_menu');
    if (!$cache) {
      // Nothing in cache so initialize it with these roles.
      $fetch = $cache_roles;
    }
    else {
      $crumb_cache = $cache->data;
    }
  }

  if (!isset($crumb_cache[$cache_roles])) {
    // No cache for this set of roles.
    $fetch = $cache_roles;
  }
  else {
    $breadcrumbs = $crumb_cache[$cache_roles];
  }

  if (!$breadcrumbs) {
    $fetch = $cache_roles;
  }

  if (isset($fetch)) {
    module_load_include('inc', 'islandora_solr_search', 'includes/query_processor');
    $solr_build = new IslandoraSolrQueryProcessor();

    $content_model = variable_get('islandora_solr_content_model_field', 'RELS_EXT_hasModel_uri_ms');

    $solr_query = "*:*";
    $solr_params = array(
      "fq" => "{$content_model}:\"info:fedora/islandora:collectionCModel\"",
      "fl" => "PID",
      "rows" => 0,
      "facet" => "true",
      "hl" => "false",
      "facet.pivot.mincount" => 1,
      "facet.mincount" => 1,
      "facet.pivot" => "RELS_EXT_isMemberOfCollection_uri_ms,PID,fgs_label_s",
    );

    $solr_build->buildQuery($solr_query, $solr_params);
    $solr_build->solrParams = array_replace_recursive($solr_build->solrParams, $solr_params);

    // Custom replacements to make the query faster.
    $solr_build->solrParams['rows'] = 0;
    unset($solr_build->solrParams['facet.field']);

    try {
      $solr_build->executeQuery(FALSE);
      $results = (array) $solr_build->islandoraSolrResult['facet_counts']['facet_pivot'][$solr_params['facet.pivot']];
    }
    catch (Exception $e) {
      $results = array();
      drupal_set_message(check_plain(t('Error searching Solr index')) . ' ' . $e->getMessage(), 'error', FALSE);
    }

    $breadcrumbs = array();
    if (count($results) > 0) {
      foreach ($results as $parent) {
        if ($parent['count'] > 0) {
          foreach ($parent['pivot'] as $child) {
            $label = 'missing label';
            if ($child['count'] > 0) {
              $label = $child['pivot'][0]['value'];
            }
            $breadcrumbs[$child['value']] = array('parent' => $parent['value'], 'label' => $label);
          }
        }
      }
      $crumb_cache[$cache_roles] = $breadcrumbs;
      cache_set(ISLANDORA_SOLR_BREADCRUMB_CACHE_NAME, $crumb_cache, 'cache_menu');
    }
  }
  return $breadcrumbs;
}

/**
 * Builds an array of drupal links for use in breadcrumbs.
 *
 * @todo Make fully recursive...
 *
 * @param string $pid
 *   The object id whose parent will be fetched for the next link.
 * @param array $context
 *   An associative array of context for internal use when recursing. Currently
 *   only used to track a single value:
 *   - level: The number of child-parent relationships to follow. Defaults to
 *     10.
 *
 * @return array
 *   An array of links representing the breadcrumb trail, "root" first.
 */
function islandora_solr_get_breadcrumbs_recursive($pid, array &$context = NULL) {

  if ($context === NULL) {
    $context['level'] = 10;
  }

  if (!isset($context['root'])) {
    $root = variable_get('islandora_repository_pid', 'islandora:root');
    $context['root'] = $root;
  }

  if ($pid == $context['root']) {
    $title = 'Islandora Repository';
    $mlid = db_select('menu_links', 'ml')
      ->condition('ml.link_path', 'islandora')
      ->fields('ml', array('mlid'))
      ->execute()
      ->fetchField();

    if ($mlid) {
      $link = menu_link_load($mlid);
      $title = (isset($link['title']) ? $link['title'] : $title);
    }

    return array(
      l(t('Home'), '<front>'),
      l($title, 'islandora'),
    );
  }
  else {
    $object = islandora_object_load($pid);
    if ($object && !in_array('islandora:collectionCModel', $object->models)) {
      // Recurse with parents up to.
      $parents = islandora_solr_get_parents_from_rels_ext($object);

      if (count($parents) > 0 && $context['level'] > 0) {
        $parent_object = reset($parents);
        $parent = $parent_object->id;
        $this_label = $object->label;
      }
    }
    else {
      $map = islandora_solr_load_crumbs();
      if (array_key_exists($pid, $map)) {
        $parent = str_replace('info:fedora/', '', $map[$pid]['parent']);
        $this_label = $map[$pid]['label'];
      }
    }

    if (empty($this_label)) {
      $this_label = t('-');
    }

    if (isset($parent)) {
      // We found a parent.
      $context['level']--;
      return array_merge(islandora_solr_get_breadcrumbs_recursive($parent, $context), array(
        l($this_label, "islandora/object/$pid"),
      ));
    }
    else {
      // Add an non-link, as we don't know how to get back to the root, and
      // render the last two links and break (on the next pass).
      return array_merge(
        islandora_solr_get_breadcrumbs_recursive($context['root'], $context),
        array('...')
      );
    }
  }
}

/**
 * Gets any objects that the given object has a parent relationship with.
 *
 * @param AbstractObject $object
 *   The object whose parents will be returned.
 *
 * @author Diego Pino
 * Parent relationships are not defined here, so we assume any
 *
 * This function gets its info from the RELS-EXT directly rather than through an
 * risearch.
 *
 * @return array
 *   An array of FedoraObject's that the given object has a
 *   (RELS-EXT) relationship with.
 */
function islandora_solr_get_parents_from_rels_ext(AbstractObject $object) {
  try {
    $parentobjects = $object->relationships->get(FEDORA_RELS_EXT_URI, NULL, NULL, RELS_TYPE_URI);
  }
  catch (RepositoryException $e) {
    return array();
  }
  $map = function ($o) {
    if (($o['predicate']['namespace'] != 'info:fedora/fedora-system:def/model#') && ($o['object']['literal'] != TRUE)) {
      // Don't count fedora model relations or literals.
      return islandora_object_load($o['object']['value']);
    }
  };
  $parentobjects = array_map($map, $parentobjects);
  return array_filter(array_unique($parentobjects));
}
