// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-manager unit tests. Unlike
 * extension_manager_tests.js, these tests are not interacting with the real
 * chrome.developerPrivate API. */
cr.define('extension_manager_tests', function() {
  /** @enum {string} */
  var TestNames = {
    ItemOrder: 'item order',
    UpdateItemData: 'update item data',
  };

  class TestService extends TestBrowserProxy {
    constructor() {
      super([
        'getProfileConfiguration',
        'getExtensionsInfo',
      ]);
      this.itemStateChangedTarget = new FakeChromeEvent();
      this.profileStateChangedTarget = new FakeChromeEvent();
    }

    getProfileConfiguration() {
      return Promise.resolve({inDeveloperMode: false});
    }

    getItemStateChangedTarget() {
      return this.itemStateChangedTarget;
    }

    getProfileStateChangedTarget() {
      return this.profileStateChangedTarget;
    }

    getExtensionsInfo() {
      this.methodCalled('getExtensionsInfo');
      return Promise.resolve([]);
    }
  }

  function getDataByName(list, name) {
    return assert(list.find(function(el) { return el.name == name; }));
  }

  suite('ExtensionManagerUnitTest', function() {
    /** @type {extensions.Manager} */
    var manager;

    /** @type {TestService} */
    var service;

    /** @param {string} viewElement */
    function assertViewActive(tagName) {
      expectTrue(!!manager.$.viewManager.querySelector(`${tagName}.active`));
    }

    setup(function() {
      PolymerTest.clearBody();

      service = new TestService();
      extensions.Service.instance_ = service;

      manager = document.createElement('extensions-manager');
      document.body.appendChild(manager);

      return service.whenCalled('getExtensionsInfo');
    });

    test(assert(TestNames.ItemOrder), function() {
      expectEquals(0, manager.extensions_.length);

      var alphaFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'Alpha', id: 'a'.repeat(32)});
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: alphaFromStore,
      });

      expectEquals(1, manager.extensions_.length);
      expectEquals(alphaFromStore.id, manager.extensions_[0].id);

      // Unpacked extensions come first.
      var betaUnpacked = extension_test_util.createExtensionInfo(
          {location: 'UNPACKED', name: 'Beta', id: 'b'.repeat(32)});
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: betaUnpacked,
      });

      expectEquals(2, manager.extensions_.length);
      expectEquals(betaUnpacked.id, manager.extensions_[0].id);
      expectEquals(alphaFromStore.id, manager.extensions_[1].id);

      // Extensions from the same location are sorted by name.
      var gammaUnpacked = extension_test_util.createExtensionInfo(
          {location: 'UNPACKED', name: 'Gamma', id: 'c'.repeat(32)});
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: gammaUnpacked,
      });

      expectEquals(3, manager.extensions_.length);
      expectEquals(betaUnpacked.id, manager.extensions_[0].id);
      expectEquals(gammaUnpacked.id, manager.extensions_[1].id);
      expectEquals(alphaFromStore.id, manager.extensions_[2].id);

      // The name-sort should be case-insensitive, and should fall back on
      // id.
      var aaFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'AA', id: 'd'.repeat(32)});
      var AaFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'Aa', id: 'e'.repeat(32)});
      var aAFromStore = extension_test_util.createExtensionInfo(
          {location: 'FROM_STORE', name: 'aA', id: 'f'.repeat(32)});
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: aaFromStore,
      });
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: AaFromStore,
      });
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: aAFromStore,
      });

      expectEquals(6, manager.extensions_.length);
      expectEquals(betaUnpacked.id, manager.extensions_[0].id);
      expectEquals(gammaUnpacked.id, manager.extensions_[1].id);
      expectEquals(aaFromStore.id, manager.extensions_[2].id);
      expectEquals(AaFromStore.id, manager.extensions_[3].id);
      expectEquals(aAFromStore.id, manager.extensions_[4].id);
      expectEquals(alphaFromStore.id, manager.extensions_[5].id);
    });

    test(assert(TestNames.UpdateItemData), function() {
      var oldDescription = 'old description';
      var newDescription = 'new description';
      var extension = extension_test_util.createExtensionInfo(
          {description: oldDescription});
      var secondExtension = extension_test_util.createExtensionInfo({
        description: 'irrelevant',
        id: 'b'.repeat(32),
      });
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: extension,
      });
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.INSTALLED,
        extensionInfo: secondExtension,
      });
      var data = manager.extensions_[0];
      // The detail view is not present until navigation.
      expectFalse(!!manager.$$('extensions-detail-view'));
      // TODO(scottchen): maybe testing too many things in a single unit test.
      extensions.navigation.navigateTo(
          {page: Page.DETAILS, extensionId: extension.id});
      var detailsView = manager.$$('extensions-detail-view');
      expectTrue(!!detailsView);  // View should now be present.
      expectEquals(extension.id, detailsView.data.id);
      expectEquals(oldDescription, detailsView.data.description);
      expectEquals(
          oldDescription,
          detailsView.$$('.section .section-content').textContent.trim());

      var extensionCopy = Object.assign({}, extension);
      extensionCopy.description = newDescription;
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.PREFS_CHANGED,
        extensionInfo: extensionCopy,
      });
      expectEquals(extension.id, detailsView.data.id);
      expectEquals(newDescription, detailsView.data.description);
      expectEquals(
          newDescription,
          detailsView.$$('.section .section-content').textContent.trim());

      // Updating a different extension shouldn't have any impact.
      var secondExtensionCopy = Object.assign({}, secondExtension);
      secondExtensionCopy.description = 'something else';
      service.itemStateChangedTarget.callListeners({
        event_type: chrome.developerPrivate.EventType.PREFS_CHANGED,
        extensionInfo: secondExtensionCopy,
      });
      expectEquals(extension.id, detailsView.data.id);
      expectEquals(newDescription, detailsView.data.description);
      expectEquals(
          newDescription,
          detailsView.$$('.section .section-content').textContent.trim());
    });
  });

  return {
    TestNames: TestNames,
  };
});
