// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
cr.define('policy', function() {
  /**
   * A singelton object that handles communication between browser and WebUI.
   * @constructor
   */
  function Page() {}

  // Make Page a singleton.
  cr.addSingletonGetter(Page);

  /**
   * Orivude a list of all known policies to the UI. Called by the browser on
   * page load.
   * @param {Object} names Dictionary containing all known policy names.
   */
  Page.setPolicyNames = function(names) {
    var page = this.getInstance();

    // Clear all policy tables.
    page.mainSection.innerHTML = '';
    page.policyTables = {};

    // Create tables and set known policy names for Chrome and extensions.
    if (names.hasOwnProperty('chromePolicyNames')) {
      var table = page.appendNewTable('chrome', 'Chrome policies', '');
      table.setPolicyNames(names.chromePolicyNames);
    }
    if (names.hasOwnProperty('extensionPolicyNames')) {
      for (var ext in names.extensionPolicyNames) {
        var table = page.appendNewTable(
            'extension-' + ext, names.extensionPolicyNames[ext].name,
            'ID: ' + ext);
        table.setPolicyNames(names.extensionPolicyNames[ext].policyNames);
      }
    }
  };

  /**
   * Provide a list of policies and any errors detected while parsing these to
   * the UI. Called by the browser on session load.
   * @param {Object} values Dictionary containing policy values for the current
   * session.
   */
  Page.setPolicyValues = function(values) {
    var page = this.getInstance();
    // Set Chrome policies.
    if (values.hasOwnProperty('chromePolicies')) {
      var table = page.policyTables['chrome'];
      table.setPolicyValues(values.chromePolicies);
    }

    // Set extension policies.
    if (values.hasOwnProperty('extensionPolicies')) {
      for (var extensionId in values.extensionPolicies) {
        var table = page.policyTables['extension-' + extensionId];
        if (table) {
          // So far, we support only extensions that are currently installed.
          // For them the table should exist because it is created in
          // setPolicyNames.
          table.setPolicyValues(values.extensionPolicies[extensionId]);
        }
      }
    }
  };

  Page.prototype = {
    /**
     * Main initialization function. Called by the browser on page load.
     */
    initialize: function() {
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
    },

    /**
     * Creates a new policy table section, adds the section to the page,
     * and returns the new table from that section.
     * @param {string} id The key for storing the new table in policyTables.
     * @param {string} label_title Title for this policy table.
     * @param {string} label_content Description for the policy table.
     * @return {Element} The newly created table.
     */
    appendNewTable: function(id, label_title, label_content) {
      var newSection =
          this.createPolicyTableSection(id, label_title, label_content);
      this.mainSection.appendChild(newSection);
      return this.policyTables[id];
    },
    /**
     * Creates a new section containing a title, description and table of
     * policies.
     * @param {id} id The key for storing the new table in policyTables.
     * @param {string} label_title Title for this policy table.
     * @param {string} label_content Description for the policy table.
     * @return {Element} The newly created section.
     */
    createPolicyTableSection: function(id, label_title, label_content) {
      var section = document.createElement('section');
      section.setAttribute('class', 'policy-table-section');

      // Add title and description.
      var title = window.document.createElement('h3');
      title.textContent = label_title;
      section.appendChild(title);

      if (label_content) {
        var description = window.document.createElement('div');
        description.classList.add('table-description');
        description.textConetnt = label_content;
        section.appendChild(description);
      }

      // Add 'No Policies Set' element.
      var noPolicies = window.document.createElement('div');
      noPolicies.classList.add('no-policies-set');
      noPolicies.textContent = loadTimeData.getString('noPoliciesSet');
      section.appendChild(noPolicies);

      // Add table of policies.
      var newTable = this.createPolicyTable();
      this.policyTables[id] = newTable;
      section.appendChild(newTable);
      return section;
    },

    /**
     * Creates a new table for displaying policies.
     * @return {Element} The newly created table.
     */
    createPolicyTable: function() {
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
      cr.ui.decorate(newTable, PolicyTable);
      return newTable;
    },

    /**
     * Extracts current policy values to send to backend for logging.
     * @return {Object} The dictionary containing policy values.
     */
    getDictionary: function() {
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
    }
  };

  /**
   * A single policy entry in the policy table.
   * @constructor
   * @extends {HTMLTableSectionElement}
   */
  var Policy = cr.ui.define(function() {
    var node = $('policy-template').cloneNode(true);
    node.removeAttribute('id');
    return node;
  });

  Policy.prototype = {
    // Set up the prototype chain.
    __proto__: HTMLTableSectionElement.prototype,

    /**
     * Initialization function for the cr.ui framework.
     */
    decorate: function() {
      this.updateToggleExpandedValueText_();
      this.querySelector('.edit-button')
          .addEventListener('click', this.startValueEditing_.bind(this));
      this.querySelector('.value-edit-form').onsubmit =
          this.updateValue_.bind(this);
      this.querySelector('.toggle-expanded-value')
          .addEventListener('click', this.toggleExpandedValue_.bind(this));
    },

    /**
     * Populate the table columns with information about the policy name, value
     * and status.
     * @param {string} name The policy name.
     * @param {Object} value Dictionary with information about the policy value.
     * @param {boolean} unknown Whether the policy name is not recognized.
     */
    initialize: function(name, value, unknown) {
      this.name = name;
      this.unset = !value;
      this.unknown = unknown;
      this.querySelector('.name').textContent = name;
      if (value) {
        this.setValue_(value.value);
      }
      this.setStatus_(value);
    },

    /**
     * Set the status column.
     * @param {Object} value Dictionary with information about the policy value.
     * @private
     */
    setStatus_: function(value) {
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
    },

    /**
     * Set the policy value.
     * @param {Object} value Dictionary with information about the policy value.
     * @private
     */
    setValue_: function(value) {
      this.unset = !value;
      if (value) {
        this.value = value;
        this.querySelector('.value').textContent = value;
        this.querySelector('.expanded-value').textContent = value;
        this.querySelector('.value-edit-field').value = value;
      }
    },

    /**
     * Check the table columns for overflow. Most columns are automatically
     * elided when overflow occurs. The only action required is to add a tooltip
     * that shows the complete content. The value column is an exception. If
     * overflow occurs here, the contents is replaced with a link that toggles
     * the visibility of an additional row containing the complete value.
     *
     * @param {boolean} opt_value_updated If the value of the policy has been
     * updated recently, we need to recompute its width.
     */
    checkOverflow: function(opt_value_updated) {
      // Set a tooltip on all overflowed columns except the value column.
      var divs = this.querySelectorAll('div.elide');
      for (var i = 0; i < divs.length; i++) {
        var div = divs[i];
        div.title = div.offsetWidth < div.scrollWidth ? div.textContent : '';
      }

      // Cache the width of the value column's contents when it is first shown
      // or updated. This is required to be able to check whether the contents
      // would still overflow the column once it has been hidden and replaced by
      // a link.
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
    },
    /**
     * Update the text of the link that toggles the visibility of an additional
     * row containing the complete policy value, depending on the toggle state.
     * @private
     */
    updateToggleExpandedValueText_: function() {
      this.querySelector('.toggle-expanded-value').textContent =
          loadTimeData.getString(
              this.classList.contains('show-overflowed-value') ?
                  'hideExpandedValue' :
                  'showExpandedValue');
    },

    /**
     * Toggle the visibility of an additional row containing the complete policy
     * value.
     * @private
     */
    toggleExpandedValue_: function() {
      this.classList.toggle('show-overflowed-value');
      this.updateToggleExpandedValueText_();
    },

    /**
     * Start editing value.
     * @private
     */
    startValueEditing_: function() {
      this.classList.add('value-editing-on');
      this.classList.remove('has-overflowed-value');
      this.querySelector('.value-edit-field').select();
    },

    /**
     * Update the value once its editing is finished.
     */
    updateValue_: function() {
      var newValue = this.querySelector('.value-edit-field').value;
      this.setValue_(newValue);
      this.setStatus_(newValue);
      this.classList.remove('value-editing-on');
      this.checkOverflow(true);
      // this.parentNode.filter();
      var showUnset = $('show-unset').checked;
      this.hidden = this.unset && !showUnset ||
          this.name.toLowerCase().indexOf(this.parentNode.filterPattern_) == -1;
      chrome.send('updateSession', [Page.getInstance().getDictionary()]);
      return false;
    }
  };

  /**
   * A table of policies and their values.
   * @constructor
   * @extends {HTMLTableElement}
   */
  var PolicyTable = cr.ui.define('tbody');

  PolicyTable.prototype = {
    // Set up the prototype chain.
    __proto__: HTMLTableElement.prototype,

    /**
     * Initialization function for the cr.ui framework.
     */
    decorate: function() {
      this.policies_ = {};
      this.filterPattern_ = '';
      window.addEventListener('resize', this.checkOverflow_.bind(this));
    },

    /**
     * Initialize the list of all known policies.
     * @param {Object} names Dictionary containing all known policy names.
     */
    setPolicyNames: function(names) {
      this.policies_ = names;
      this.setPolicyValues({});
    },

    /**
     * Populate the table with the currently set policy values and any errors
     * detected while parsing these.
     * @param {Object} values Dictionary containing the current policy values.
     */
    setPolicyValues: function(values) {
      // Remove all policies from the table.
      var policies = this.getElementsByTagName('tbody');
      while (policies.length > 0) {
        this.removeChild(policies.item(0));
      }

      // Add known policies whose value is currently set.
      // This is needed in order to conveniently hide the unset policies.
      var unset = [];
      for (var name in this.policies_) {
        if (name in values) {
          this.setPolicyValue_(name, values[name], false);
        } else {
          unset.push(name);
        }
      }

      // Add policies whose value is set, but whose name is not recognized.
      for (var name in values) {
        if (!(name in this.policies_)) {
          this.setPolicyValue_(name, values[name], true);
        }
      }
      // Add unset policies.
      for (var i = 0; i < unset.length; i++) {
        this.setPolicyValue_(unset[i], undefined, false);
      }

      // Filter the policies.
      this.filter();
    },

    /**
     * Set the filter pattern. Only policies whose name contains |pattern| are
     * shown in the policy table. The filter is case insensitive. It can be
     * disabled by setting |pattern| to an empty string.
     * @param {string} pattern The filter pattern.
     */
    setFilterPattern: function(pattern) {
      this.filterPattern_ = pattern.toLowerCase();
      this.filter();
    },

    /**
     * Check the table columns for overflow.
     * @private
     */
    checkOverflow_: function() {
      var policies = this.getElementsByTagName('tbody');
      for (var i = 0; i < policies.length; i++) {
        if (!policies[i].hidden) {
          policies[i].checkOverflow();
        }
      }
    },

    /**
     * Add a policy with the given |name| and |value| to the table.
     * @param {string} name The policy name.
     * @param {Object} value Dictionary with information about the policy value.
     * @param {boolean} unknown Whether the policy name is not recoginzed.
     * @private
     */
    setPolicyValue_: function(name, value, unknown) {
      var policy = new Policy;
      policy.initialize(name, value, unknown);
      this.appendChild(policy);
    },
    /**
     * Filter policies. Only policies whose name contains the filter pattern are
     * shown in the table. Furthermore, policies whose value is not currently
     * set are only shown if the corresponding checkbox is checked.
     */
    filter: function() {
      var showUnset = $('show-unset').checked;
      var policies = this.getElementsByTagName('tbody');
      for (var i = 0; i < policies.length; i++) {
        var policy = policies[i];
        policy.hidden = policy.unset && !showUnset ||
            policy.name.toLowerCase().indexOf(this.filterPattern_) == -1;
      }
      if (this.querySelector('tbody:not([hidden])'))
        this.parentElement.classList.remove('empty');
      else
        this.parentElement.classList.add('empty');
      setTimeout(this.checkOverflow_.bind(this), 0);
    },
    /**
     * Get policy values stored in this table for backend.
     */
    getDictionary: function() {
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
    }
  };

  return {Page: Page};
});

// Have the main initialization function be called when the page finishes
// loading.
document.addEventListener(
    'DOMContentLoaded',
    policy.Page.getInstance().initialize.bind(policy.Page.getInstance()));