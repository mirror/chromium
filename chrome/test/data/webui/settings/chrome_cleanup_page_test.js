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
    console.log('Called dismissCleanupPage');
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

    chromeCleanupPage = document.createElement('settings-chrome-cleanup-page');
    document.body.appendChild(chromeCleanupPage);
  });

  teardown(function() {
    chromeCleanupPage.remove();
    updateLoadTimeData(false);
  });

  var updateLoadTimeData = function(userInitiatedCleanupsEnabled) {
    loadTimeData.overrideValues({
      userInitiatedCleanupsEnabled: userInitiatedCleanupsEnabled,
    });
  };

  // var startCleanupFromInfected = function(userInitiatedCleanupsEnabled) {
  //   updateLoadTimeData(userInitiatedCleanupsEnabled);

  //   cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', false);
  //   cr.webUIListenerCallback(
  //       'chrome-cleanup-on-infected', ['file 1', 'file 2', 'file 3']);
  //   Polymer.dom.flush();

  //   var showFilesButton = chromeCleanupPage.$$('#show-files-button');
  //   assertTrue(!!showFilesButton);
  //   // MockInteractions.tap(showFilesButton);
  //   filesToRemove = chromeCleanupPage.$$('#files-to-remove-list');
  //   assertTrue(!!filesToRemove);
  //   assertEquals(filesToRemove.getElementsByTagName('li').length, 3);

  //   var actionButton = chromeCleanupPage.$$('#action-button');
  //   assertTrue(!!actionButton);
  //   // MockInteractions.tap(actionButton);
  //   ChromeCleanupProxy.whenCalled('startCleanup').then(
  //       function(logsUploadEnabled) {
  //         assertFalse(logsUploadEnabled);
  //         cr.webUIListenerCallback('chrome-cleanup-on-cleaning', false);
  //         Polymer.dom.flush();

  //         var spinner = chromeCleanupPage.$$('#cleaning-spinner');
  //         assertTrue(spinner.active);
  //       });
  // };

  // test('startCleanupFromInfected_UserInitiatedCleanupsDisabled', function() {
  //   startCleanupFromInfected(false);
  // });

  // test('startCleanupFromInfected_UserInitiatedCleanupsEnabled', function() {
  //   startCleanupFromInfected(true);
  // });

  // var rebootFromRebootRequired = function(userInitiatedCleanupsEnabled) {
  //   updateLoadTimeData(userInitiatedCleanupsEnabled);

  //   cr.webUIListenerCallback('chrome-cleanup-on-reboot-required');
  //   Polymer.dom.flush();

  //   var actionButton = chromeCleanupPage.$$('#action-button');
  //   assertTrue(!!actionButton);
  //   // MockInteractions.tap(actionButton);
  //   return ChromeCleanupProxy.whenCalled('restartComputer');
  // };

  // test('rebootFromRebootRequired_UserInitiatedCleanupsDisabled', function() {
  //   console.log('new line');
  //   rebootFromRebootRequired(false);
  // });

  // test('rebootFromRebootRequired_UserInitiatedCleanupsEnabled', function() {
  //   rebootFromRebootRequired(true);
  // });

  var dismissFromIdleCleaningFailure = function(userInitiatedCleanupsEnabled) {
    updateLoadTimeData(userInitiatedCleanupsEnabled);

    cr.webUIListenerCallback('chrome-cleanup-on-cleaning', false);
    cr.webUIListenerCallback(
        'chrome-cleanup-on-idle',
        settings.ChromeCleanupIdleReason.CLEANING_FAILED);
    Polymer.dom.flush();

    var actionButton = chromeCleanupPage.$$('#action-button');
    assertTrue(!!actionButton);
    // MockInteractions.tap(actionButton);
    return ChromeCleanupProxy.whenCalled('dismissCleanupPage');
  };

  test('dismissFromIdleCleaningFailure_UserInitiatedCleanupsDisabled', function() {
    debugger;
    dismissFromIdleCleaningFailure(false);
  });

  // test('dismissFromIdleCleaningFailure_UserInitiatedCleanupsEnabled', function() {
  //   debugger;
  //   dismissFromIdleCleaningFailure(true);
  // });

  // var dismissFromIdleCleaningSuccess = function(userInitiatedCleanupsEnabled) {
  //   console.log(loadTimeData.getBoolean('userInitiatedCleanupsEnabled'));

  //   updateLoadTimeData(userInitiatedCleanupsEnabled);

  //   cr.webUIListenerCallback('chrome-cleanup-on-cleaning', false);
  //   cr.webUIListenerCallback(
  //       'chrome-cleanup-on-idle',
  //       settings.ChromeCleanupIdleReason.CLEANING_SUCCEEDED);
  //   Polymer.dom.flush();

  //   var actionButton = chromeCleanupPage.$$('#action-button');
  //   assertTrue(!!actionButton);
  //   MockInteractions.tap(actionButton);
  //   return ChromeCleanupProxy.whenCalled('dismissCleanupPage');
  // };

  // test('dismissFromIdleCleaningSuccess_UserInitiatedCleanupsDisabled', function() {
  //   dismissFromIdleCleaningSuccess(false);
  // });

  // test('dismissFromIdleCleaningSuccess_UserInitiatedCleanupsEnabled', function() {
  //   dismissFromIdleCleaningSuccess(true);
  // });

  // var setLogsUploadPermission = function(userInitiatedCleanupsEnabled) {
  //   updateLoadTimeData(userInitiatedCleanupsEnabled);

  //   cr.webUIListenerCallback(
  //       'chrome-cleanup-on-infected', ['file 1', 'file 2', 'file 3']);
  //   Polymer.dom.flush();

  //   var logsControl = chromeCleanupPage.$$('#chromeCleanupLogsUploadControl');
  //   assertTrue(!!logsControl);

  //   cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', true);
  //   Polymer.dom.flush();
  //   assertTrue(logsControl.checked);

  //   cr.webUIListenerCallback('chrome-cleanup-upload-permission-change', false);
  //   Polymer.dom.flush();
  //   assertFalse(logsControl.checked);

  //   // MockInteractions.tap(logsControl.$.control);
  //   return ChromeCleanupProxy.whenCalled('setLogsUploadPermission').then(
  //       function(logsUploadEnabled) {
  //         assertTrue(logsUploadEnabled);
  //       });
  // };

  // test('setLogsUploadPermission_UserInitiatedCleanupsDisabled', function() {
  //   setLogsUploadPermission(false);
  // });

  // test('setLogsUploadPermission_UserInitiatedCleanupsEnabled', function() {
  //   setLogsUploadPermission(true);
  // });
});
