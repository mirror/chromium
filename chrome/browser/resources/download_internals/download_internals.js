// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('downloadInternals', function() {
  'use strict';

  /** @type {!downloadInternals.DownloadInternalsBrowserProxy} */
  var browserProxy =
      downloadInternals.DownloadInternalsBrowserProxyImpl.getInstance();

  /** @type {!Array<ServiceEntry>} */
  var ongoingServiceEntries = [];

  /** @type {!Array<ServiceEntry>} */
  var finishedServiceEntries = [];

  /** @type {!Array<ServiceRequest>} */
  var serviceRequests = [];

  function removeGuidFromList(list, guid) {
    for (var i = 0; i < list.length; i++) {
      if (list[i].guid == guid) {
        list.splice(i, 1);
        break;
      }
    }
  }

  function addOrUpdateEntryByGuid(list, entry) {
    for (var i = 0; i < list.length; i++) {
      if (list[i].guid == entry.guid) {
        list[i] = entry;
        return;
      }
    }
    list.unshift(entry);
  }

  function updateEntryTables() {
    var ongoingInput = new JsEvalContext({entries: ongoingServiceEntries});
    jstProcess(ongoingInput, $('download-service-ongoing-entries-info'));

    var finishedInput = new JsEvalContext({entries: finishedServiceEntries});
    jstProcess(finishedInput, $('download-service-finished-entries-info'));
  }

  function onServiceStatusChanged(state) {
    $('service-state').textContent = state.serviceState;
    $('service-status-model').textContent = state.modelStatus;
    $('service-status-driver').textContent = state.driverStatus;
    $('service-status-file').textContent = state.fileMonitorStatus;
  }

  function onServiceDownloadsAvailable(entries) {
    for (var i = 0; i < entries.length; i++) {
      var entry = entries[i];
      if (entry.state == 'COMPLETE') {
        finishedServiceEntries.unshift(entry);
      } else {
        ongoingServiceEntries.unshift(entry);
      }
    }

    updateEntryTables();
  }

  function onServiceDownloadChanged(entry) {
    if (entry.state == 'COMPLETE') {
      removeGuidFromList(ongoingServiceEntries, entry.guid);
      addOrUpdateEntryByGuid(finishedServiceEntries, entry);
    } else {
      addOrUpdateEntryByGuid(ongoingServiceEntries, entry);
    }

    updateEntryTables();
  }

  function onServiceDownloadFailed(entry) {
    removeGuidFromList(ongoingServiceEntries, entry.guid);
    addOrUpdateEntryByGuid(finishedServiceEntries, entry);

    updateEntryTables();
  }

  function onServiceRequestMade(request) {
    serviceRequests.unshift(request);
    var input = new JsEvalContext({requests: serviceRequests});
    jstProcess(input, $('download-service-request-info'));
  }

  function initialize() {
    // Register all event listeners.
    cr.addWebUIListener('service-status-changed', onServiceStatusChanged);
    cr.addWebUIListener(
        'service-downloads-available', onServiceDownloadsAvailable);
    cr.addWebUIListener('service-download-changed', onServiceDownloadChanged);
    cr.addWebUIListener('service-download-failed', onServiceDownloadFailed);
    cr.addWebUIListener('service-request-made', onServiceRequestMade);

    // Kick off requests for the current system state.
    browserProxy.getServiceStatus().then(onServiceStatusChanged);
    browserProxy.getServiceDownloads().then(onServiceDownloadsAvailable);
  }

  return {
    initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', downloadInternals.initialize);