// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for interventions_internals.js
 */

/** @const {string} Path to source root. */
let ROOT_PATH = '../../../../';

/**
 * Test fixture for InterventionsInternals WebUI testing.
 * @constructor
 * @extends testing.Test
 */
function InterventionsInternalsUITest() {
  this.setupFnResolver = new PromiseResolver();
}

InterventionsInternalsUITest.prototype = {
  __proto__: testing.Test.prototype,

  /**
   * Browse to the options page and call preLoad().
   * @override
   */
  browsePreload: 'chrome://interventions-internals',

  /** @override */
  isAsync: true,

  extraLibraries: [
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
    ROOT_PATH + 'ui/webui/resources/js/promise_resolver.js',
    ROOT_PATH + 'ui/webui/resources/js/util.js',
    ROOT_PATH + 'chrome/test/data/webui/test_browser_proxy.js',
  ],

  preLoad: function() {
    /**
     * A stub class for the Mojo PageHandler.
     */
    class TestPageHandler extends TestBrowserProxy {
      constructor() {
        super(['getPreviewsEnabled']);

        /** @private {!Map} */
        this.statuses_ = new Map();
      }

      /**
       * Setup testing map.
       * @param {!Map} map The testing status map.
       */
      setTestingMap(map) {
        this.statuses_ = map;
      }

      /** @override **/
      getPreviewsEnabled() {
        this.methodCalled('getPreviewsEnabled');
        return Promise.resolve({
          statuses: this.statuses_,
        });
      }
    }

    window.setupFn = function() {
      return this.setupFnResolver.promise;
    }.bind(this);

    window.testPageHandler = new TestPageHandler();

    /**
     * Convert milliseconds to human readable date/time format.
     * The return format will be "MM/dd/YYYY hh:mm:ss.sss"
     * @param {number} time Time in millisecond since Unix Epoch.
     * @return The converted string format.
     */
    getTimeFormat = function(time) {
      let date = new Date(time);
      let options = {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
      }

      let timeString = date.toLocaleDateString('en-US', options);
      return dateString + ' ' + date.getHours() + ':' + date.getMinutes() +
          ':' + date.getSeconds() + '.' + date.getMilliseconds();
    };

    getBlacklistedStatus = function(blacklisted, time) {
      let formattedTime = getTimeFormat(time);
      return 'User blacklisted status: ' +
          (blacklisted ? 'Blacklisted ' : 'Not blacklisted ') + '(at ' +
          formattedTime + ')';
    };
  },
};

TEST_F('InterventionsInternalsUITest', 'DisplayCorrectStatuses', function() {
  let setupFnResolver = this.setupFnResolver;

  test('DisplayCorrectStatuses', () => {
    // Setup testPageHandler behavior.
    let testMap = new Map();
    testMap.set('params1', {
      description: 'Params 1',
      enabled: true,
    });
    testMap.set('params2', {
      description: 'Params 2',
      enabled: false,
    });
    testMap.set('params3', {
      description: 'Param 3',
      enabled: false,
    });

    window.testPageHandler.setTestingMap(testMap);
    this.setupFnResolver.resolve();

    return setupFnResolver.promise
        .then(() => {
          return window.testPageHandler.whenCalled('getPreviewsEnabled');
        })
        .then(() => {
          testMap.forEach((value, key) => {
            let expected = value.description + ': ' +
                (value.enabled ? 'Enabled' : 'Disabled');
            let actual = document.querySelector('#' + key).textContent;
            expectEquals(expected, actual);
          });
        });
  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'LogNewMessage', function() {
  test('LogMessageIsPostedCorrectly', () => {
    let pageImpl = new InterventionsInternalPageImpl(null);
    let logs = [
      {
        type: 'Type_a',
        description: 'Some description_a',
        url: {url: 'Some gurl.spec()_a'},
        time: 1507221689240,  // Oct 05 2017 16:41:29 UTC
      },
      {
        type: 'Type_b',
        description: 'Some description_b',
        url: {url: 'Some gurl.spec()_b'},
        time: 758675653000,  // Jan 15 1994 23:14:13 UTC
      },
      {
        type: 'Type_c',
        description: 'Some description_c',
        url: {url: 'Some gurl.spec()_c'},
        time: -314307870000,  // Jan 16 1960 04:15:30 UTC
      },
    ];

    logs.forEach((log) => {
      pageImpl.logNewMessage(log);
    });

    let logTable = $('message-logs-table');
    let rows = logTable.querySelectorAll('.log-message');
    expectEquals(logs.length, rows.length);

    logs.forEach((log, index) => {
      let expectedTime = getTimeFormat(log.time);
      let row = rows[index];

      expectEquals(expectedTime, row.querySelector('.log-time').textContent);
      expectEquals(log.type, row.querySelector('.log-type').textContent);
      expectEquals(
          log.description, row.querySelector('.log-description').textContent);
      expectEquals(log.url.url, row.querySelector('.log-url').textContent);
    });

  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'AddNewBlacklistedHost', function() {
  test('AddNewBlacklistedHost', () => {
    let pageImpl = new InterventionsInternalPageImpl(null);
    let time = 758675653000;  // Jan 15 1994 23:14:13 UTC
    let expectedHost = 'example.com';
    pageImpl.onBlacklistedHost(expectedHost, time);

    let blacklistedTable = $('blacklisted-hosts-table');
    let row = blacklistedTable.querySelector('.blacklisted-host-row');

    let expectedTime = getTimeFormat(time);

    expectEquals(
        expectedHost, row.querySelector('.host-blacklisted').textContent);
    expectEquals(
        expectedTime, row.querySelector('.host-blacklisted-time').textContent);

  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'HostAlreadyBlacklisted', function() {
  test('HostAlreadyBlacklisted', () => {
    let pageImpl = new InterventionsInternalPageImpl(null);
    let time0 = 758675653000;   // Jan 15 1994 23:14:13 UTC
    let time1 = 1507221689240;  // Oct 05 2017 16:41:29 UTC
    let expectedHost = 'example.com';

    pageImpl.onBlacklistedHost(expectedHost, time0);

    let blacklistedTable = $('blacklisted-hosts-table');
    let row = blacklistedTable.querySelector('.blacklisted-host-row');
    let expectedTime = getTimeFormat(time0);
    expectEquals(
        expectedHost, row.querySelector('.host-blacklisted').textContent);
    expectEquals(
        expectedTime, row.querySelector('.host-blacklisted-time').textContent);

    pageImpl.onBlacklistedHost(expectedHost, time1);

    // The row remains the same.
    expectEquals(
        expectedHost, row.querySelector('.host-blacklisted').textContent);
    expectEquals(
        expectedTime, row.querySelector('.host-blacklisted-time').textContent);
  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'UpdateUserBlacklisted', function() {
  test('UpdateUserBlacklistedDisplayCorrectly', () => {
    let pageImpl = new InterventionsInternalPageImpl(null);
    let time0 = 758675653000;   // Jan 15 1994 23:14:13 UTC
    let time1 = 1507221689240;  // Oct 05 2017 16:41:29 UTC
    let time2 = 1514797323000;  // Jan 01 2018 09:02:03 UTC
    let time3 = 1582999872000;  // Feb 29 2020 18:11:12 UTC

    pageImpl.onUserBlacklisted(time0);
    let state = $('user-blacklisted-status');
    expectEquals(
        getBlacklistedStatus(true /* blacklisted */, time0), state.textContent);

    pageImpl.onUserNotBlacklisted(time1);
    expectEquals(
        getBlacklistedStatus(false /* blacklisted */, time1),
        state.textContent);

    pageImpl.onUserBlacklisted(time2);
    expectEquals(
        getBlacklistedStatus(true /* blacklisted */, time2), state.textContent);

    // The blacklisted time display when the user is blacklisted.
    pageImpl.onUserBlacklisted(time3);
    expectEquals(
        getBlacklistedStatus(true /* blacklisted */, time2), state.textContent);
  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'UpdateBlacklistCleared', function() {
  test('UpdatePageWhenBlacklistIsCleared', () => {
    let pageImpl = new InterventionsInternalPageImpl(null);
    let hosts =
        ['example_3.com', 'example_1.com', 'example_0.net', 'example_2.vn'];

    let hostBlacklistedTime = 758675653000;    // Jan 15 1994 23:14:13 UTC
    let userBlacklistedTime = 1507221689240;   // Oct 05 2017 16:41:29 UTC
    let blacklistClearedTime = 1582999872000;  // Feb 29 2020 18:11:12 UTC

    hosts.forEach((host) => {
      pageImpl.onBlacklistedHost(host, hostBlacklistedTime);
    });
    pageImpl.onUserBlacklisted(userBlacklistedTime);

    let blacklistedTable = $('blacklisted-hosts-table');
    let rows = blacklistedTable.querySelectorAll('.blacklisted-host-row');

    expectEquals(hosts.length, rows.length);

    let state = $('user-blacklisted-status');
    expectEquals(
        getBlacklistedStatus(true /* blacklisted */, userBlacklistedTime),
        state.textContent);

    pageImpl.onBlacklistCleared(blacklistClearedTime);
    let updatedRows =
        blacklistedTable.querySelectorAll('.blacklisted-host-row');

    // All hosts are removed from table.
    expectEquals(0, updatedRows.length);

    // User blacklisted state is reset.
    expectEquals(
        getBlacklistedStatus(false /* blacklisted */, blacklistClearedTime),
        state.textContent);

    let expectedClearedTime = getTimeFormat(blacklistClearedTime);
    let blacklistClearedStatus =
        'Last blacklist cleared: ' + expectedClearedTime;
    let actualClearedStatus = $('blacklist-cleared-status');
    expectEquals(blacklistClearedStatus, actualClearedStatus.textContent);
  });

  mocha.run();
});
