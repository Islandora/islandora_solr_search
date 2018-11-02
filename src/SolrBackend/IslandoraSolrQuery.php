<?php
/**
 * @file
 * Base class to store re-used variable and deprecation.
 */

namespace Drupal\islandora_solr\SolrBackend;

/**
 * Class IslandoraSolrQuery
 * @package Drupal\islandora_solr\SolrBackend
 */
abstract class IslandoraSolrQuery implements IslandoraSolrQueryInterface {

  /**
   * Different kinds of nothing
   */
  protected $differentKindsOfNothing = array(
    ' ',
    '%20',
    '%252F',
    '%2F',
    '%252F-',
    '',
  );

  /**
   * Handle deprectation of old class member gracefully.
   */
  public function __get($name) {
    $map = array(
      'different_kinds_of_nothing' => 'differentKindsOfNothing',
    );

    if (isset($map[$name])) {
      $new_name = $map[$name];
      $trace = debug_backtrace();

      $message = t('Use of variable name "@class->@old_name" on line @line of @file deprecated as of version @version. Refactor to use "@class->@name" before the next release.', array(
        '@old_name' => $name,
        '@name' => $new_name,
        '@class' => __CLASS__,
        '@version' => '7.x-1.2',
        '@line' => $trace[0]['line'],
        '@file' => $trace[0]['file'],
      ));

      trigger_error($message, E_USER_DEPRECATED);

      return $this->$new_name;
    }
  }

}