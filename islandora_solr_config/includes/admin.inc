<?php

/**
 * @file
 * Contains all administration related functionality
 */

/**
 * Function to return admin setting form.
 * @return array
 *   returns the form
 */
function islandora_solr_config_admin_rss_settings($form, &$form_state) {

  // Get variables.
  $rss_item = variable_get('islandora_solr_config_rss_item', array(
    'title' => 'fgs_label_s',
    'description' => '',
    'author' => '',
    'category' => '',
    'pubDate' => 'fgs_createdDate_s',
    'enclosure_dsid' => 'TN',
    ));
  $rss_channel = variable_get('islandora_solr_config_rss_channel', array(
    'copyright' => '',
    'managingEditor' => '',
    'webMaster' => '',
  ));

  $form = array(
    '#tree' => TRUE,
  );

  $form['rss_item'] = array(
    '#type' => 'fieldset',
    '#title' => t('Item settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('To take full control over the RSS item output you can also override the following method: IslandoraSolrResultsRSS::rssItem()'),
  );
  $form['rss_item']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('Solr field to populate the item title.'),
    '#default_value' => $rss_item['title'],
    '#required' => TRUE,
  );
  $form['rss_item']['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#description' => t('Solr field to populate the item synopsis.'),
    '#default_value' => $rss_item['description'],
  );
  $form['rss_item']['author'] = array(
    '#type' => 'textfield',
    '#title' => t('Author'),
    '#description' => t('Solr field to populate the item author.'),
    '#default_value' => $rss_item['author'],
  );
  $form['rss_item']['category'] = array(
    '#type' => 'textfield',
    '#title' => t('Category'),
    '#description' => t('Solr field to populate the item category.'),
    '#default_value' => $rss_item['category'],
  );
  $form['rss_item']['pubDate'] = array(
    '#type' => 'textfield',
    '#title' => t('Publication date'),
    '#description' => t('Solr field to populate the item publication date (pubDate).'),
    '#default_value' => $rss_item['pubDate'],
  );
  $form['rss_item']['enclosure_dsid'] = array(
    '#type' => 'textfield',
    '#title' => t('Enclosure (datastream ID)'),
    '#description' => t('Fill out a datastream ID to be added as a media enclosure. Defaults to thumbnail (TN).'),
    '#default_value' => $rss_item['enclosure_dsid'],
  );
  $form['rss_channel'] = array(
    '#type' => 'fieldset',
    '#title' => t('Channel settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('To take full control over the RSS channel output you can also override the following method: IslandoraSolrResultsRSS::rssChannel()'),
  );
  $form['rss_channel']['copyright'] = array(
    '#type' => 'textfield',
    '#title' => t('Copyright'),
    '#description' => t('Copyright notice for content in the channel.'),
    '#default_value' => $rss_channel['copyright'],
  );
  $form['rss_channel']['managingEditor'] = array(
    '#type' => 'textfield',
    '#title' => t('Managing editor'),
    '#description' => t('Email address for person responsible for editorial content.'),
    '#default_value' => $rss_channel['managingEditor'] ? $rss_channel['managingEditor'] : '',
  );
  $form['rss_channel']['webMaster'] = array(
    '#type' => 'textfield',
    '#title' => t('Webmaster'),
    '#description' => t('Email address for person responsible for technical issues relating to channel.'),
    '#default_value' => $rss_channel['webMaster'] ? $rss_channel['webMaster'] : '',
  );
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 50,
  );
  $form['buttons']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset to defaults'),
    '#weight' => 51,
  );

  return $form;
}

/**
 * Function to submit the admin setting form.
 */
function islandora_solr_config_admin_rss_settings_submit($form, &$form_state) {

  // On save.
  if ($form_state['clicked_button']['#value'] == t('Save')) {

    // Get values.
    $rss_item = $form_state['values']['rss_item'];
    $rss_channel = $form_state['values']['rss_channel'];

    // Set variable.
    variable_set('islandora_solr_config_rss_item', $rss_item);
    variable_set('islandora_solr_config_rss_channel', $rss_channel);
  }

  // On reset.
  if ($form_state['clicked_button']['#value'] == t('Reset to defaults')) {
    variable_del('islandora_solr_config_rss_item');
    variable_del('islandora_solr_config_rss_channel');
  }
}
