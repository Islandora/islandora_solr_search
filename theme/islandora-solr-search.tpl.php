<?php

/**
* @file islandora-solr-search.tpl.php
* Islandora solr search primary results template file
*
* Variables available:
* - $variables: all array elements of $variables can be used as a variable. e.g. $base_url equals $variables['base_url']
* - $base_url: The base url of the current website. eg: http://example.com .
* - $user: The user object.
*
* - $results: Primary profile results array
*/

?>

<?php if (empty($results)): ?>
  <p><?php print t('Sorry, but your search returned no results.'); ?></p>
<?php else: ?>
  <!-- Ordered list -->
  <ol class="islandora-solr-search-results" start="<?php print $solr_start + 1 ?>">
    <?php $row_result = 0; ?>
    <?php foreach($results as $result): ?>
    <!-- add first/last classes + zebra -->
    <li class="<?php print $row_result % 2 == 0 ? 'odd' : 'even'; ?>">
      <!-- Table -->
      <table class="islandora-solr-search-result">
        <?php $row_field = 0; ?>
        <?php foreach($result as $key => $value): ?>
          <tr class="<?php print $value['class']; ?> <?php print $row_field % 2 == 0 ? 'odd' : 'even'; ?>">
            <th class="solr-label"><?php print $value['label']; ?></th>
            <?php if ($key == 'PID'): ?>
              <?php $value['value'] = l($value['value'], 'fedora/repository/' . htmlspecialchars($value['value'], ENT_QUOTES, 'utf-8')); ?>
            <?php endif; ?>
            <td class="solr-value"><?php print $value['value']; ?></td>
          </tr>
          <?php $row_field++; ?>
        <?php endforeach; ?>
      </table>
    </li>
    <?php $row_result++; ?>
    <?php endforeach; ?>
  </ol>
<?php endif; ?>