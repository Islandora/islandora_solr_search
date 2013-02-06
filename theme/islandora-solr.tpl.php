<?php
/**
 * @file
 *   Islandora solr search primary results template file
 *
 * Variables available:
 * - $results: Primary profile results array
 *
 * @see template_preprocess_islandora_solr()
 */

?>

<?php if (empty($results)): ?>
  <p class="no-results"><?php print t('Sorry, but your search returned no results.'); ?></p>
  <?php else: ?>
  <div class="islandora islandora-solr-search-results">
    <?php
      $row_result = 0;
      foreach($results as $key => $result):
    ?>
      <!-- Search result -->
      <div class="islandora-solr-search-result clear-block <?php print $row_result % 2 == 0 ? 'odd' : 'even'; ?>">
        <!-- Thumbnail -->
        <dl class="solr-thumb">
          <dt>
            <?php
              $image = '<img src="' . url($result['thumbnail_url'], array('query' => $result['thumbnail_url_params'])) . '" />';
              print l($image, $result['object_url'], array('html' => TRUE, 'query' => $result['object_url_params']));
            ?>
          </dt>
          <dd></dd>
        </dl>
        <!-- Metadata -->
        <dl class="solr-fields islandora-inline-metadata">
          <?php
            $row_field = 0;
            $max_rows = count($results[$row_result]) - 1;
            foreach($result['solr_doc'] as $key => $value): ?>
            <dt class="solr-label
              <?php
                print $value['class'];
                print $row_field == 0 ? ' first' : '';
                print $row_field == $max_rows ? ' last' : '';
              ?>">
              <?php print $value['label']; ?>
            </dt>
            <?php
              if ($key == 'PID'):
                $value['value'] = l($value['value'], $result['object_url'], array('query' => $result['object_url_params']));
              endif;
            ?>
            <dd class="solr-value <?php print $value['class']; ?><?php print $row_field == 0 ? ' first' : ''; ?><?php print $row_field == $max_rows ? ' last' : ''; ?>">
              <?php print $value['value']; ?>
            </dd>
            <?php $row_field++; ?>
          <?php endforeach; ?>
        </dl>
      </div>
      <?php
        $row_result++;
      endforeach;
      ?>
  </div>
<?php endif; ?>
