<?php
/**
 * @file
 * Islandora Solr grid template
 *
 * Variables available:
 * - $results: Primary profile results array
 *
 * @see template_preprocess_islandora_solr_grid()
 */
?>

<?php if (empty($results)): ?>
  <p class="no-results"><?php print t('Sorry, but your search returned no results.'); ?></p>
<?php else: ?>
  <div class="islandora-solr-search-results">
    <div class="islandora-solr-grid clearfix">
    <?php foreach($results as $result): ?>
      <dl class="solr-grid-field">
        <dt class="solr-grid-thumb">
          <?php
            $image = '<img src="' . url($result['thumbnail_url'], array('query' => $result['thumbnail_url_params'])) . '" />';
            print l($image, $result['object_url'], array(
              'html' => TRUE,
              'query' => $result['object_url_params'],
              'fragment' => isset($result['object_url_fragment']) ? $result['object_url_fragment'] : '',
            ));
          ?>
        </dt>
        <?php foreach($result['solr_doc'] as $key => $value): ?>
          <?php if ($key != $value['label']): ?>
            <dt class="solr-label <?php print $value['class']; ?>">
              <?php print $value['label']; ?>
            </dt>
          <?php endif; ?>
          <dd class="solr-grid-caption <?php print $value['class']; ?>">
            <?php print $value['value']; ?>
          </dd>
        <?php endforeach; ?>
      </dl>
    <?php endforeach; ?>
    </div>
  </div>
<?php endif; ?>
