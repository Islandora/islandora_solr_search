<?php

/**
* @file islandora-solr-wrapper.tpl.php
* Islandora solr search results wrapper template
*
* Variables available:
* - $variables: all array elements of $variables can be used as a variable. e.g. $base_url equals $variables['base_url']
* - $base_url: The base url of the current website. eg: http://example.com .
* - $user: The user object.
*
* - $secondary_profiles: Rendered secondary profiles
* - $results: Rendered search results (primary profile)
* - $islandora_solr_result_count: Solr result count string
* - $solrpager: The pager
* - $solr_debug: debug info
*
*/
?>

<div id="islandora-solr-top">
  <div id='islandora_solr_secondary_display_profiles'><?php print $secondary_profiles; ?></div>
  <div id="islandora_solr_result_count"><?php print $islandora_solr_result_count; ?></div>
</div>
<div class="islandora-solr-content content">
  <?php print $solr_pager; ?>
  <?php print $results; ?>
  <?php print $solr_debug; ?>
  <?php print $solr_pager; ?>
</div>