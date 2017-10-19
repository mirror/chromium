// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** The columns that are used to find rows that contain the keyword. */
const KEY_COLUMNS = ['log-type', 'log-description', 'log-url'];

/**
 * Switch the selected tab to 'selected-tab' class.
 */
function setSelectedTab() {
  let selected =
      document.querySelector('input[type=radio][name=tabs]:checked').value;
  let selectedTab = document.querySelector('#' + selected);
  selectedTab.className =
      selectedTab.className.replace('hidden-tab', 'selected-tab');
}

/**
 * Change the previously selected element to 'hidden-tab' class, and switch the
 * selected element to 'selected-tab' class.
 */
function changeTab() {
  let lastSelected = document.querySelector('.selected-tab');
  lastSelected.className =
      lastSelected.className.replace('selected-tab', 'hidden-tab');

  setSelectedTab();
}

/**
 * Initialize the navigation bar, and setup OnChange listeners for the tabs.
 */
function setupTabControl() {
  // Initialize on change listeners.
  let tabs = document.querySelectorAll('input[type=radio][name=tabs]');
  tabs.forEach((tab) => {
    tab.addEventListener('change', changeTab);
  });

  let tabContents = document.querySelectorAll('.tab-content');
  tabContents.forEach((tab) => {
    tab.className += ' hidden-tab';
  });

  // Turn on the default selected tab.
  setSelectedTab();
}

/**
 * Initialize the search functionality of the search bar on the log tab.
 * Searching will hide any rows that don't contain the keyword in the search
 * bar.
 */
function setupLogSearch() {
  $('log-search-bar').addEventListener('keyup', () => {
    let keyword = $('log-search-bar').value.toUpperCase();
    let rows = document.querySelectorAll('.log-message');

    rows.forEach((row) => {
      let found = KEY_COLUMNS.some((column) => {
        return (row.querySelector('.' + column)
                    .textContent.toUpperCase()
                    .includes(keyword));
      });
      row.style.display = found ? '' : 'none';
    });
  });
}

/** @constructor */
let InterventionsInternalPageImpl = function(request, blacklistedHosts) {
  this.blacklistedHosts_ = blacklistedHosts;
  this.binding_ =
      new mojo.Binding(mojom.InterventionsInternalsPage, this, request);
  this.userBlacklisted = null;
};

InterventionsInternalPageImpl.prototype = {
  /**
   * Post a new log message to the web page.
   *
   * @override
   * @param {!MessageLog} log The new log message recorded by
   * PreviewsLogger.
   */
  logNewMessage: function(log) {
    let logsTable = $('message-logs-table');
    let tableRow = document.createElement('tr');
    tableRow.setAttribute('class', 'log-message');

    let timeTd = document.createElement('td');
    let date = new Date(log.time);
    timeTd.textContent = date.toISOString();
    timeTd.setAttribute('class', 'log-time');
    tableRow.appendChild(timeTd);

    let typeTd = document.createElement('td');
    typeTd.setAttribute('class', 'log-type');
    typeTd.textContent = log.type;
    tableRow.appendChild(typeTd);

    let descriptionTd = document.createElement('td');
    descriptionTd.setAttribute('class', 'log-description');
    descriptionTd.textContent = log.description;
    tableRow.appendChild(descriptionTd);

    // TODO(thanhdle): Truncate url and show full url when user clicks on it.
    // crbug.com/773019
    let urlTd = document.createElement('td');
    urlTd.setAttribute('class', 'log-url');
    urlTd.textContent = log.url.url;
    tableRow.appendChild(urlTd);

    logsTable.appendChild(tableRow);
  },

  /**
   * Update new blacklisted host to the web page.
   *
   * @override
   * @param {!string} host The blacklisted host.
   * @param {number} time The time of the event in milliseconds since Unix
   * epoch.
   */
  updateBlacklistedHost: function(host, time) {
    if (this.blacklistedHosts_.has(host)) {
      // Host is blacklisted already.
      return;
    }

    let row = document.createElement('tr');
    row.setAttribute('class', 'blacklisted-host-row');

    let hostTd = document.createElement('td');
    hostTd.setAttribute('class', 'host-blacklisted');
    hostTd.textContent = host;
    row.appendChild(hostTd);

    let timeTd = document.createElement('td');
    timeTd.setAttribute('class', 'host-blacklisted-time');
    let date = new Date(time);
    timeTd.textContent = date.toISOString();
    row.appendChild(timeTd);

    // TODO(thanhdle): Insert row at correct index. crbug.com/776105.
    $('blacklisted-hosts-table').appendChild(row);

    // Put the new row in the map of blacklisted hosts.
    this.blacklistedHosts_.set(host, row);
  },

  /**
   * Update to the page that the user is blacklisted at given time.
   *
   * @override
   * @param {number} time The time of the event in milliseconds since Unix
   * epoch.
   */
  updateUserBlacklisted: function(time) {
    if (this.userBlacklisted) {
      // Do not update when user is already blacklisted.
      return;
    }
    let userBlacklistedStatus = $('user-blacklisted-status');
    let date = new Date(time);
    userBlacklistedStatus.textContent =
        'User blacklisted status: Blacklisted ' +
        '(at ' + date.toISOString() + ')';
    this.userBlacklisted = true;
  },

  /**
   * Update to the page that the user is not blacklisted at given time.
   *
   * @override
   * @param {number} time The time of the event in milliseconds since Unix
   * epoch.
   */
  updateUserNotBlacklisted: function(time) {
    let userBlacklistedStatus = $('user-blacklisted-status');
    let date = new Date(time);
    userBlacklistedStatus.textContent =
        'User blacklisted status: Not blacklisted ' +
        '(last update: ' + date.toISOString() + ')';
    this.userBlacklisted = false;
  },

  /**
   * Update the blacklist cleared status on the page.
   *
   * @override
   * @param {number} time The time of the event in milliseconds since Unix
   * epoch.
   */
  updateBlacklistCleared: function(time) {
    let blacklistClearedStatus = $('blacklist-cleared-status');
    let date = new Date(time);
    blacklistClearedStatus.textContent =
        'Last blacklist cleared: ' + date.toISOString();

    // Remove hosts from table.
    let blacklistedHostsTable = $('blacklisted-hosts-table');
    let rows = blacklistedHostsTable.querySelectorAll('.blacklisted-host-row');
    rows.forEach((_) => {
      blacklistedHostsTable.deleteRow(1);
    });

    this.updateUserNotBlacklisted(time);
  },
};

cr.define('interventions_internals', () => {
  let pageHandler = null;

  function init(handler) {
    pageHandler = handler;
    getPreviewsEnabled();
  }

  /**
   * Retrieves the statuses of previews (i.e. Offline, LoFi, AMP Redirection),
   * and posts them on chrome://intervention-internals.
   */
  function getPreviewsEnabled() {
    pageHandler.getPreviewsEnabled()
        .then((response) => {
          let statuses = $('previews-statuses');

          // TODO(thanhdle): The statuses are not printed in alphabetic order of
          // the key. crbug.com/772458
          response.statuses.forEach((value, key) => {
            let message = value.description + ': ';
            message += value.enabled ? 'Enabled' : 'Disabled';

            assert(!$(key), 'Component ' + key + ' already existed!');

            let node = document.createElement('p');
            node.setAttribute('id', key);
            node.textContent = message;
            statuses.appendChild(node);
          });
        })
        .catch((error) => {
          console.error(error.message);
        });
  }

  return {
    init: init,
  };
});

window.setupFn = window.setupFn || function() {
  return Promise.resolve();
};

document.addEventListener('DOMContentLoaded', () => {
  setupTabControl();
  setupLogSearch();
  let pageHandler = null;
  let pageImpl = null;

  window.setupFn().then(() => {
    if (window.testPageHandler) {
      pageHandler = window.testPageHandler;
    } else {
      pageHandler = new mojom.InterventionsInternalsPageHandlerPtr;
      Mojo.bindInterface(
          mojom.InterventionsInternalsPageHandler.name,
          mojo.makeRequest(pageHandler).handle);

      // Set up client side mojo interface.
      let client = new mojom.InterventionsInternalsPagePtr;
      pageImpl = new InterventionsInternalPageImpl(
          mojo.makeRequest(client), new Map());
      pageHandler.setClientPage(client);
    }

    interventions_internals.init(pageHandler);
  });
});
