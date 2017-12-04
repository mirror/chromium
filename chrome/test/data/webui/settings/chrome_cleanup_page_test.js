// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @implements {settings.ChromeCleanupProxy} */
class TestChromeCleanupProxy extends TestBrowserProxy {
  constructor() {
    super([
      'dismissCleanupPage',
      'registerChromeCleanerObserver',
      'restartComputer',
      'setLogsUploadPermission',
      'startCleanup',
      'startScanning',
      'notifyShowDetails',
      'notifyLearnMoreClicked',
    ]);
  }

  /** @override */
  dismissCleanupPage(source) {
    this.methodCalled('dismissCleanupPage', source);
  }

  /** @override */
  registerChromeCleanerObserver() {
    this.methodCalled('registerChromeCleanerObserver');
  }

  /** @override */
  restartComputer() {
    this.methodCalled('restartComputer');
  }

  /** @override */
  setLogsUploadPermission(enabled) {
    this.methodCalled('setLogsUploadPermission', enabled);
  }

  /** @override */
  startCleanup(logsUploadEnabled) {
    this.methodCalled('startCleanup', logsUploadEnabled);
  }

  /** @override */
  startScanning(logsUploadEnabled) {
    this.methodCalled('startScanning', logsUploadEnabled);
  }

  /** @override */
  notifyShowDetails(enabled) {
    this.methodCalled('notifyShowDetails', enabled);
  }

  /** @override */
  notifyLearnMoreClicked() {
    this.methodCalled('notifyLearnMoreClicked');
  }
}

var chromeCleanupPage = null;

/** @type {?TestDownloadsBrowserProxy} */
var ChromeCleanupProxy = null;

suite('ChromeCleanupHandler', function() {
  setup(function() {
    ChromeCleanupProxy = new TestChromeCleanupProxy();
    settings.ChromeCleanupProxyImpl.instance_ = ChromeCleanupProxy;

    PolymerTest.clearBody();
  });

  teardown(function() {
    chromeCleanupPage.remove();
    loadTimeData.overrideValues({
      userInitiatedCleanupsEnabled: false,
    });
  });

  var initParametrizedTest = function(userInitiatedCleanupsEnabled) {
    loadTimeData.overrideValues({
      userInitiatedCleanupsEnabled: userInitiatedCleanupsEnabled,
    });

    chromeCleanupPage = document.createElement('settings-chrome-cleanup-page');
    document.body.appendChild(chromeCleanupPage);
  };

  var scanOfferedOnInitiallyIdle = function(idleReason) {
    initParametrizedTest(true);

    cr.webUIListenerCallback('chrome-cleanup-on-idle', idleReason);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertTrue(!!actionButton);
  };

  test('scanOfferedOnInitiallyIdle_ReporterFoundNothing', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.REPORTER_FOUND_NOTHING);
  });

  test('scanOfferedOnInitiallyIdle_ReporterFailed', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.REPORTER_FAILED);
  });

  test('scanOfferedOnInitiallyIdle_ScanningFoundNothing', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.SCANNING_FOUND_NOTHING);
  });

  test('scanOfferedOnInitiallyIdle_ScanningFailed', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.SCANNING_FAILED);
  });

  test('scanOfferedOnInitiallyIdle_ConnectionLost', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.CONNECTION_LOST);
  });

  test('scanOfferedOnInitiallyIdle_UserDeclinedCleanup', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.USER_DECLINED_CLEANUP);
  });

  test('scanOfferedOnInitiallyIdle_CleaningFailed', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.CLEANING_FAILED);
  });

  test('scanOfferedOnInitiallyIdle_CleaningSucceeded', function() {
    scanOfferedOnInitiallyIdle(
        settings.ChromeCleanupIdleReason.CLEANING_SUCCEEDED);
  });

  test('reporterFoundNothing', function() {
    initParametrizedTest(true);

    cr.webUIListenerCallback('chrome-cleanup-on-reporter-running');
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle',
        settings.ChromeCleanupIdleReason.REPORTER_FOUND_NOTHING);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertFalse(!!actionButton);
  });

  test('reporterFoundNothing', function() {
    initParametrizedTest(true);

    cr.webUIListenerCallback('chrome-cleanup-on-reporter-running');
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle',
        settings.ChromeCleanupIdleReason.REPORTER_FOUND_NOTHING);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertFalse(!!actionButton);
  });

  test('startScanFromIdle', function() {
    initParametrizedTest(true);

    cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', false);
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle', settings.ChromeCleanupIdleReason.INITIAL);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertTrue(!!actionButton);
    MockInteractions.tap(actionButton);
    return ChromeCleanupProxy.whenCalled('startScanning')
        .then(function(logsUploadEnabled) {
          assertFalse(logsUploadEnabled);
          cr.webUIListenerCallback('chrome-cleanup-on-scanning', false);
          Polymer.dom.flush();

          var spinner = chromeCleanupPage.$$('#waiting-spinner');
          assertTrue(spinner.active);
        });
  });

  test('scanFoundNothing', function() {
    initParametrizedTest(true);

    cr.webUIListenerCallback('chrome-cleanup-on-scanning', false);
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle',
        settings.ChromeCleanupIdleReason.SCANNING_FOUND_NOTHING);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertFalse(!!actionButton);
  });

  test('scanFailure', function() {
    initParametrizedTest(true);

    cr.webUIListenerCallback('chrome-cleanup-on-scanning', false);
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle',
        settings.ChromeCleanupIdleReason.SCANNING_FAILED);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertFalse(!!actionButton);
  });

  var startCleanupFromInfected = function(userInitiatedCleanupsEnabled) {
    initParametrizedTest(userInitiatedCleanupsEnabled);

    cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', false);
    cr.webUIListenerCallback(
        'chrome-cleanup-on-infected', ['file 1', 'file 2', 'file 3']);
    Polymer.dom.flush();

    var showFilesButton = chromeCleanupPage.$$('#show-files-button');
    assertTrue(!!showFilesButton);
    MockInteractions.tap(showFilesButton);
    filesToRemove = chromeCleanupPage.$$('#files-to-remove-list');
    assertTrue(!!filesToRemove);
    assertEquals(filesToRemove.getElementsByTagName('li').length, 3);

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertTrue(!!actionButton);
    MockInteractions.tap(actionButton);
    return ChromeCleanupProxy.whenCalled('startCleanup')
        .then(function(logsUploadEnabled) {
          assertFalse(logsUploadEnabled);
          cr.webUIListenerCallback('chrome-cleanup-on-cleaning', false);
          Polymer.dom.flush();

          var spinner = chromeCleanupPage.$$('#waiting-spinner');
          assertTrue(spinner.active);
        });
  };

  test('startCleanupFromInfected_UserInitiatedCleanupsDisabled', function() {
    return startCleanupFromInfected(false);
  });

  test('startCleanupFromInfected_UserInitiatedCleanupsEnabled', function() {
    return startCleanupFromInfected(true);
  });

  var rebootFromRebootRequired = function(userInitiatedCleanupsEnabled) {
    initParametrizedTest(userInitiatedCleanupsEnabled);

    cr.webUIListenerCallback('chrome-cleanup-on-reboot-required');
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertTrue(!!actionButton);
    MockInteractions.tap(actionButton);
    return ChromeCleanupProxy.whenCalled('restartComputer');
  };

  test('rebootFromRebootRequired_UserInitiatedCleanupsDisabled', function() {
    return rebootFromRebootRequired(false);
  });

  test('rebootFromRebootRequired_UserInitiatedCleanupsEnabled', function() {
    return rebootFromRebootRequired(true);
  });

  var cleanupFailure = function(userInitiatedCleanupsEnabled) {
    initParametrizedTest(userInitiatedCleanupsEnabled);

    cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', false);
    cr.webUIListenerCallback('chrome-cleanup-on-cleaning', false);
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle',
        settings.ChromeCleanupIdleReason.CLEANING_FAILED);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    if (userInitiatedCleanupsEnabled) {
      assertFalse(!!actionButton);
    } else {
      assertTrue(!!actionButton);
      MockInteractions.tap(actionButton);
      return ChromeCleanupProxy.whenCalled('dismissCleanupPage');
    }
  };

  test('cleanupFailure_UserInitiatedCleanupsDisabled', function() {
    return cleanupFailure(false);
  });

  test('cleanupFailure_UserInitiatedCleanupsEnabled', function() {
    return cleanupFailure(true);
  });

  var cleanupSuccess = function(userInitiatedCleanupsEnabled) {
    initParametrizedTest(userInitiatedCleanupsEnabled);

    cr.webUIListenerCallback('chrome-cleanup-on-cleaning', false);
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle',
        settings.ChromeCleanupIdleReason.CLEANING_SUCCEEDED);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    if (userInitiatedCleanupsEnabled) {
      assertFalse(!!actionButton);
    } else {
      assertTrue(!!actionButton);
      MockInteractions.tap(actionButton);
      return ChromeCleanupProxy.whenCalled('dismissCleanupPage');
    }
  };

  test('cleanupSuccess_UserInitiatedCleanupsDisabled', function() {
    return cleanupSuccess(false);
  });

  test('cleanupSuccess_UserInitiatedCleanupsEnabled', function() {
    return cleanupSuccess(true);
  });

  var testLogsUploading = function(
      testingScanOffered, userInitiatedCleanupsEnabled) {
    initParametrizedTest(userInitiatedCleanupsEnabled);

    if (testingScanOffered) {
      cr.webUIListenerCallback(
          'chrome-cleanup-on-infected', ['file 1', 'file 2', 'file 3']);
    } else {
      cr.webUIListenerCallback(
          'chrome-cleanup-on-idle', settings.ChromeCleanupIdleReason.INITIAL);
    }
    Polymer.dom.flush();

    var logsControl = chromeCleanupPage.$$('#chromeCleanupLogsUploadControl');
    assertTrue(!!logsControl);

    cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', true);
    Polymer.dom.flush();
    assertTrue(logsControl.checked);

    cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', false);
    Polymer.dom.flush();
    assertFalse(logsControl.checked);

    MockInteractions.tap(logsControl.$.control);
    return ChromeCleanupProxy.whenCalled('setLogsUploadPermission').then(
        function(logsUploadEnabled) {
          assertTrue(logsUploadEnabled);
        });
  };

  test('logsUploadingOnScanOffered_UserInitiatedCleanupsDisabled', function() {
    return testLogsUploading(false, false);
  });

  test('logsUploadingOnScanOffered_UserInitiatedCleanupsEnabled', function() {
    return testLogsUploading(false, true);
  });

  test('logsUploadingOnInfected_UserInitiatedCleanupsDisabled', function() {
    return testLogsUploading(true, false);
  });

  test('logsUploadingOnInfected_UserInitiatedCleanupsEnabled', function() {
    return testLogsUploading(true, true);
  });
});
