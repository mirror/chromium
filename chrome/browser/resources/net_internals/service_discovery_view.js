// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** This view displays service discovery transactions.  Service discovery is
    provided by the internal mDNS implementation on most platforms except MacOS,
    where it is provided by the NetService framework. */
var ServiceDiscoveryView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function ServiceDiscoveryView() {
    assertFirstConstructorCall(ServiceDiscoveryView);

    // Call superclass's constructor.
    superClass.call(this, ServiceDiscoveryView.MAIN_BOX_ID);

    $(ServiceDiscoveryView.START_BUTTON_ID)
        .addEventListener('click', this.onStartButtonClicked_.bind(this));
    $(ServiceDiscoveryView.SERVICE_NAME_INPUT_ID)
        .addEventListener('input', this.onServiceNameChanged_.bind(this));
    g_browser.addServiceDiscoveryObserver(this);
  }

  ServiceDiscoveryView.TAB_ID = 'tab-handle-service-discovery';
  ServiceDiscoveryView.TAB_NAME = 'Service Discovery';
  ServiceDiscoveryView.TAB_HASH = '#service-discovery';

  // IDs for special HTML elements in service_discovery_view.html
  ServiceDiscoveryView.MAIN_BOX_ID = 'service-discovery-view-tab-content';
  ServiceDiscoveryView.SERVICE_NAME_INPUT_ID = 'service-discovery-name-input';
  ServiceDiscoveryView.START_BUTTON_ID = 'service-discovery-start-button';
  ServiceDiscoveryView.EVENTS_TBODY_ID = 'service-discovery-events-tbody';

  cr.addSingletonGetter(ServiceDiscoveryView);

  ServiceDiscoveryView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    // Are we currently running discovery?
    running_: false,

    onStartButtonClicked_: function() {
      if (this.running_) {
        this.running_ = false;
        $(ServiceDiscoveryView.START_BUTTON_ID).innerHTML = 'Start';
        $(ServiceDiscoveryView.SERVICE_NAME_INPUT_ID).disabled = false;
        g_browser.sendStopServiceDiscovery();
      } else {
        this.running_ = true;
        $(ServiceDiscoveryView.START_BUTTON_ID).innerHTML = 'Stop';
        $(ServiceDiscoveryView.SERVICE_NAME_INPUT_ID).disabled = true;

        // Clear out any existing events when we restart.
        $(ServiceDiscoveryView.EVENTS_TBODY_ID).innerHTML = '';
        g_browser.sendStartServiceDiscovery(
            $(ServiceDiscoveryView.SERVICE_NAME_INPUT_ID).value);
      }
    },

    onServiceNameChanged_: function() {
      if ($(ServiceDiscoveryView.SERVICE_NAME_INPUT_ID).value == '') {
        $(ServiceDiscoveryView.START_BUTTON_ID).disabled = true;
      } else {
        $(ServiceDiscoveryView.START_BUTTON_ID).disabled = false;
      }
    },

    onDeviceChanged: function(record) {
      var row = $(ServiceDiscoveryView.EVENTS_TBODY_ID).insertRow(-1);
      row.insertCell(-1).innerText = '' + record.time;
      if (record.added) {
        row.insertCell(-1).innerText = 'ADDED';
      } else {
        row.insertCell(-1).innerText = 'UPDATED';
      }
      row.insertCell(-1).innerText = record.service_name;
      row.insertCell(-1).innerText = record.address;
      row.insertCell(-1).innerText = record.ip;
      row.insertCell(-1).innerText = record.last_seen;
      row.insertCell(-1).innerText = record.metadata;
    },

    onDeviceRemoved: function(record) {
      var row = $(ServiceDiscoveryView.EVENTS_TBODY_ID).insertRow(-1);
      row.insertCell(-1).innerText = '' + record.time;
      row.insertCell(-1).innerText = 'REMOVED';
      row.insertCell(-1).innerText = record.service_name;
      row.insertCell(-1).colSpan = '4';
    },

    onDeviceCacheFlushed: function(time) {
      var row = $(ServiceDiscoveryView.EVENTS_TBODY_ID).insertRow(-1);
      row.insertCell(-1).innerText = '' + time;
      row.insertCell(-1).innerText = 'CACHE FLUSH';
      row.insertCell(-1).colSpan = '5';
    }
  };

  return ServiceDiscoveryView;
})();
