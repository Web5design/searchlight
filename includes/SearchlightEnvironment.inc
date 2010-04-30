<?php

class SearchlightEnvironment {
  var $name;
  var $datasource;
  var $enabled;
  var $facets;
  var $options;

  var $query;
  var $view;

  function __construct() {
    $this->name = '';
    $this->datasource = '';
    $this->facets = array();
    $this->options = array();

    $this->query = NULL;
    $this->view = NULL;
  }

  /**
   * Activate this environment such that it can build/render facet displays.
   * Detect the current page view (if any) and clone its query object.
   */
  function activate($reset = FALSE) {
    if (!isset($this->query) || $reset) {
      $display = views_get_page_view();
      if ($display && !empty($display->view->query->searchlight)) {
        $this->query = drupal_clone($display->view->query);
        $this->view = $display->view;
      }
      else {
        $this->query = NULL;
        $this->view = NULL;
      }
    }
    return $this;
  }

  function render($type = 'active') {
    $rendered = array();
    foreach ($this->getFacets() as $name => $facet) {
      if ($plugin = $this->getPlugin($name)) {
        switch ($type) {
          case 'active':
            if (searchlight_facets()->getFacet($name)) {
              $rendered[$name] = $plugin->render($this->query, $name, $this->facets[$name]['items']);
            }
            break;
          case 'inactive':
            if (!searchlight_facets()->getFacet($name)) {
              $rendered[$name] = $plugin->render($this->query, $name, $this->facets[$name]['items']);
            }
            break;
        }
      }
    }
    return $rendered;
  }

  function label($type = 'active') {
  }

  function getPlugin($name) {
    $this->activate();
    if (!empty($this->query) && isset($this->query->datasource->fields[$name])) {
      $field = $this->query->datasource->fields[$name];
      $plugin = searchlight_get_plugin('facet', "{$field['table']}_{$field['field']}");
      // Fallback to default plugin.
      return $plugin ? $plugin : searchlight_get_plugin('facet', 'default');
    }
    return FALSE;
  }

  function getFacets() {
    if (!isset($this->enabled)) {
      $this->enabled = array();
      $this->activate();
      if (!empty($this->query)) {
        foreach ($this->facets as $name => $facet) {
          if (!empty($facet['enabled']) && isset($this->query->datasource->fields[$name])) {
            $this->enabled[$name] = $facet;
          }
        }
      }
    }
    return $this->enabled;
  }

  function optionsForm(&$form, &$form_state) {
    views_include('admin');
    views_include('form');

    $form['#theme'] = 'searchlight_admin_environment';

    $form['name'] = array(
      '#title' => t('Name'),
      '#type' => 'item',
      '#value' => $this->name,
    );

    $options = array();
    foreach (searchlight_datasource_load() as $datasource) {
      $options[$datasource->name] = $datasource->name;
    }
    $form['datasource']['#tree'] = TRUE;
    $form['datasource']['datasource'] = array(
      '#title' => t('datasource'),
      '#type' => 'select',
      '#default_value' => $this->datasource,
      '#options' => $options,
    );
    $form['datasource']['update'] = array(
      '#value' => t('Update datasource'),
      '#type' => 'submit',
      '#submit' => array('searchlight_admin_environment_edit_submit'),
    );

    $datasource = searchlight_datasource_load($this->datasource);
    if ($datasource) {
      $form['facets'] = array('#tree' => TRUE);
      foreach ($datasource->fields as $name => $field) {
        if (in_array($field['usage'], array('attribute', 'multivalue'), TRUE)) {
          $form['facets'][$name] = array(
            '#tree' => TRUE,
            '#weight' => isset($this->facets[$name]['weight']) ? $this->facets[$name]['weight'] : 0,
          );
          $form['facets'][$name]['enabled'] = array(
            '#type' => 'checkbox',
            '#default_value' => isset($this->facets[$name]['enabled']) ? $this->facets[$name]['enabled'] : TRUE,
          );
          $form['facets'][$name]['label'] = array(
            '#type' => 'markup',
            '#value' => $field['label'],
          );
          $form['facets'][$name]['weight'] = array(
            '#type' => 'weight',
            '#default_value' => isset($this->facets[$name]['weight']) ? $this->facets[$name]['weight'] : 0,
          );
          $form['facets'][$name]['items'] = array(
            '#type' => 'select',
            '#default_value' => isset($this->facets[$name]['items']) ? $this->facets[$name]['items'] : 5,
            '#options' => drupal_map_assoc(range(1, 20)),
          );
        }
      }
    }
  }

  function optionsSubmit(&$form, &$form_state) {
    switch ($form_state['clicked_button']['#id']) {
      case 'edit-datasource-update':
        $this->datasource = $form_state['values']['datasource']['datasource'];
        break;
      case 'edit-save':
        $this->facets = $form_state['values']['facets'];
        // $this->options = $form_state['values']['options'];
        break;
    }
  }
}