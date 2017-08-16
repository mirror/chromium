// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Main initialization function. Called by the browser on page load.
 */
policy.Page.prototype.initialize = function() {
  cr.ui.FocusOutlineManager.forDocument(document);

  this.mainSection = $('main-section');
  this.policyTables = {};

  // Place the initial focus on the session choice input field.
  $('session-name-field').select();

  var self = this;
  $('filter').onsearch = function(event) {
    for (policyTable in self.policyTables) {
      self.policyTables[policyTable].setFilterPattern(this.value);
    }
  };

  $('session-choice').onsubmit = function() {
    var session = $('session-name-field').value;
    chrome.send('loadSession', [session]);
    $('session-name-field').value = '';
    // Return false in order to prevent the browser from reloading the whole
    // page.
    return false;
  };

  $('show-unset').onchange = function() {
    for (policyTable in self.policyTables) {
      self.policyTables[policyTable].filter();
    }
  };

  // Notify the browser that the page has loaded, causing it to send the
  // list of all known policies and the values from default session.
  chrome.send('initializedAdmin');
};
/**
 * Extracts current policy values to send to backend for logging.
 * @return {Object} The dictionary containing policy values.
 */
policy.Page.prototype.getDictionary = function() {
  var result = {chromePolicies: {}, extensionPolicies: {}};
  for (var id in this.policyTables) {
    if (id == 'chrome') {
      result.chromePolicies = this.policyTables[id].getDictionary();
    } else {
      var extensionId = id.substr('extension-'.length);
      result.extensionPolicies[extensionId] =
          this.policyTables[id].getDictionary();
    }
  }
  return result;
};

/**
 * Creates a new table for displaying policies.
 * @return {Element} The newly created table.
 */
policy.Page.prototype.createPolicyTable = function() {
  var newTable = window.document.createElement('table');
  var tableHead = window.document.createElement('thead');
  var tableRow = window.document.createElement('tr');
  var tableHeadings = ['Name', 'Value', 'Status'];
  for (var i = 0; i < tableHeadings.length; i++) {
    var tableHeader = window.document.createElement('th');
    tableHeader.classList.add(tableHeadings[i].toLowerCase() + '-column');
    tableHeader.textContent =
        loadTimeData.getString('header' + tableHeadings[i]);
    tableRow.appendChild(tableHeader);
  }

  tableHead.appendChild(tableRow);
  newTable.appendChild(tableHead);
  cr.ui.decorate(newTable, policy.PolicyTable);
  return newTable;
};

/**
 * Initialization function for the cr.ui framework.
 */
policy.Policy.prototype.decorate = function() {
  this.updateToggleExpandedValueText_();
  this.querySelector('.edit-button')
      .addEventListener('click', this.startValueEditing_.bind(this));
  this.querySelector('.value-edit-form').onsubmit =
      this.updateValue_.bind(this);
  this.querySelector('.toggle-expanded-value')
      .addEventListener('click', this.toggleExpandedValue_.bind(this));
};

/**
 * Populate the table columns with information about the policy name,
 * value and status.
 * @param {string} name The policy name.
 * @param {Object} value Dictionary with information about the policy value.
 * @param {boolean} unknown Whether the policy name is not recognized.
 */
policy.Policy.prototype.initialize = function(name, value, unknown) {
  this.name = name;
  this.unset = !value;
  this.unknown = unknown;
  this.querySelector('.name').textContent = name;
  if (value) {
    this.setValue_(value.value);
  }
  this.setStatus_(value);
};

/**
 * Set the status column.
 * @param {Object} value Dictionary with information about the policy value.
 * @private
 */
policy.Policy.prototype.setStatus_ = function(value) {
  var status;
  if (!value) {
    status = loadTimeData.getString('unset');
  } else if (this.unknown) {
    status = loadTimeData.getString('unknown');
  } else if (value.error) {
    status = value.error;
  } else {
    status = loadTimeData.getString('ok');
  }
  this.querySelector('.status').textContent = status;
};

/** NEW. Convenience function, moving some functionality here.
 * Set the policy value.
 * @param {Object} value Dictionary with information about the policy value.
 * @private
 */
policy.Policy.prototype.setValue_ = function(value) {
  this.unset = !value;
  this.value = value;
  this.querySelector('.value').textContent = value || '';
  this.querySelector('.expanded-value').textContent = value || '';
  this.querySelector('.value-edit-field').value = value || '';
};

/**
 * Check the table columns for overflow. Most columns are automatically
 * elided when overflow occurs. The only action required is to add a
 * tooltip that shows the complete content. The value column is an
 * exception. If overflow occurs here, the contents is replaced with a
 * link that toggles the visibility of an additional row containing the
 * complete value.
 *
 * @param {boolean} opt_value_updated If the value of the policy has been
 * updated recently, we need to recompute its width.
 */
policy.Policy.prototype.checkOverflow = function(opt_value_updated) {
  // Set a tooltip on all overflowed columns except the value column.
  var divs = this.querySelectorAll('div.elide');
  for (var i = 0; i < divs.length; i++) {
    var div = divs[i];
    div.title = div.offsetWidth < div.scrollWidth ? div.textContent : '';
  }

  // Cache the width of the value column's contents when it is first shown
  // or updated. This is required to be able to check whether the contents
  // would still overflow the column once it has been hidden and replaced
  // by a link.
  var valueContainer = this.querySelector('.value-container');
  if (opt_value_updated || valueContainer.valueWidth == undefined) {
    // TODO: get rid of this dirty hack with +10.
    valueContainer.valueWidth =
        valueContainer.querySelector('.value').offsetWidth +
        valueContainer.querySelector('.edit-button').offsetWidth + 10;
  }

  // Determine whether the contents of the value column overflows. The
  // visibility of the contents, rreplacement link and additional row
  // containing the complete value that depend on this are handeld by CSS.
  if (valueContainer.offsetWidth < valueContainer.valueWidth) {
    this.classList.add('has-overflowed-value');
  } else {
    this.classList.remove('has-overflowed-value');
  }
};

/**
 * Start editing value.
 * @private
 */
policy.Policy.prototype.startValueEditing_ = function() {
  this.classList.add('value-editing-on');
  this.classList.remove('has-overflowed-value');
  this.querySelector('.value-edit-field').select();
};

/**
 * Update the value once its editing is finished.
 */
policy.Policy.prototype.updateValue_ = function() {
  var newValue = this.querySelector('.value-edit-field').value;
  this.setValue_(newValue);
  this.setStatus_(newValue);
  this.classList.remove('value-editing-on');
  this.checkOverflow(true);
  var showUnset = $('show-unset').checked;
  this.hidden = this.unset && !showUnset ||
      this.name.toLowerCase().indexOf(this.parentNode.filterPattern_) == -1;
  chrome.send('updateSession', [policy.Page.getInstance().getDictionary()]);
  return false;
};
/** NEW
 * Get policy values stored in this table for backend.
 * @returns {Object} Dictionary with policy values.
 */
policy.PolicyTable.prototype.getDictionary = function() {
  var result = {};
  var policies = this.getElementsByTagName('tbody');
  for (var i = 0; i < policies.length; i++) {
    var policy = policies[i];
    if (policy.unset) {
      continue;
    }
    result[policy.name] = {value: policy.value};
  }
  return result;
};

// Have the main initialization function be called when the page finishes
// loading.
document.addEventListener(
    'DOMContentLoaded',
    policy.Page.getInstance().initialize.bind(policy.Page.getInstance()));