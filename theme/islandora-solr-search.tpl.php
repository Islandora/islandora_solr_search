<?php

/**
* @file islandora-solr-search.tpl.php
* Islandora solr search primary results template file
*
* Variables available:
* - $variables: all array elements of $variables can be used as a variable. e.g. $base_url equals $variables['base_url']
* - $base_url: The base url of the current website. eg: http://example.com/drupal .
* - $user: The user object.
*
* - $results: Primary profile results array
*/
?>

<?php if (empty($results)): ?>
  <p class="no-results"><?php print t('Sorry, but your search returned no results.'); ?></p>
<?php else: ?>
  <div class="islandora-solr-search-results">
    <?php $row_result = 0; ?>
    <?php foreach($results as $result): ?>
    <!-- Search result -->
    <div class="islandora-solr-search-result clear-block <?php print $row_result % 2 == 0 ? 'odd' : 'even'; ?>">
      <!-- Thumbnail -->
      <dl class="solr-thumb">
        <dt><?php print $solr_thumbnail[$row_result] ?></dt>
        <dd></dd>
      </dl>
      <!-- Metadata -->
      <dl class="solr-fields">
        <?php $row_field = 0; ?>
        <?php $max_rows = count($results[$row_result]) - 1; ?>
        <?php foreach($result as $key => $value): ?>
          <dt class="solr-label <?php print $value['class']; ?><?php print $row_field == 0 ? ' first' : ''; ?><?php print $row_field == $max_rows ? ' last' : ''; ?>">
            <?php print $value['label']; ?>
          </dt>
          <?php if ($key == 'PID'): ?>
            <?php $value['value'] = l($value['value'], 'fedora/repository/' . htmlspecialchars($value['value'], ENT_QUOTES, 'utf-8')); ?>
          <?php endif; ?>
          <dd class="solr-value <?php print $value['class']; ?><?php print $row_field == 0 ? ' first' : ''; ?><?php print $row_field == $max_rows ? ' last' : ''; ?>">
            <?php print $value['value']; ?>
          </dd>
          <?php $row_field++; ?>
        <?php endforeach; ?>
      </dl>
    </div>
    <?php $row_result++; ?>
    <?php endforeach; ?>
  </div>
<?php endif; ?>