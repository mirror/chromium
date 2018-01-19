// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-kiosk-dialog. */
cr.define('extension_kiosk_mode_tests', function() {
  /** @enum {string} */
  let TestNames = {
    Layout: 'Layout',
    AddButton: 'AddButton',
    AutoLaunch: 'AutoLaunch',
    Bailout: 'Bailout',
    Updated: 'Updated',
    AddError: 'AddError',
  };

  let suiteName = 'kioskModeTests';

  suite(suiteName, function() {
    let browserProxy = null;
    let dialog = null;
    const basicApps = [
      {
        id: 'app_1',
        name: 'App1 Name',
        iconURL: '',
        autoLaunch: false,
        isLoading: false,
      },
      {
        id: 'app_2',
        name: '',  // no name
        iconURL: '',
        autoLaunch: false,
        isLoading: false,
      },
    ];

    function initPage() {
      PolymerTest.clearBody();
      browserProxy.reset();
      dialog = document.createElement('extensions-kiosk-dialog');
      document.body.appendChild(dialog);

      return browserProxy.whenCalled('getKioskAppSettings')
          .then(() => PolymerTest.flushTasks());
    }

    setup(function() {
      browserProxy = new TestKioskBrowserProxy();
      // Setup with basic apps fixtures.
      browserProxy.appSettings.apps = basicApps.slice(0);
      extensions.KioskBrowserProxyImpl.instance_ = browserProxy;

      return initPage();
    });

    test(assert(TestNames.Layout), function() {
      browserProxy.appSettings.apps[1].autoLaunch = true;
      browserProxy.appSettings.apps[1].isLoading = true;
      browserProxy.appSettings.hasAutoLaunchApp = true;

      return initPage()
          .then(() => {
            const items = dialog.shadowRoot.querySelectorAll('.list-item');
            expectEquals(items.length, 2);
            expectTrue(items[0].textContent.indexOf(basicApps[0].name) !== -1);
            expectTrue(items[1].textContent.indexOf(basicApps[1].name) !== -1);
            // Second item should show the auto-lauch label.
            expectTrue(items[0].querySelector('span').hidden);
            expectFalse(items[1].querySelector('span').hidden);
            // No permission to edit auto-launch so buttons should be hidden.
            expectTrue(items[0].querySelector('paper-button').hidden);
            expectTrue(items[1].querySelector('paper-button').hidden);
            // Bailout checkbox should be hidden when auto-launch editing
            // disabled.
            expectTrue(dialog.$$('paper-checkbox').hidden);

            MockInteractions.tap(items[0].querySelector('.icon-delete-gray'));
            Polymer.dom.flush();
            return browserProxy.whenCalled('removeKioskApp');
          })
          .then(appId => {
            expectEquals(appId, basicApps[0].id);
          });
    });

    test(assert(TestNames.AutoLaunch), function() {
      browserProxy.initialSettings.autoLaunchEnabled = true;
      browserProxy.appSettings.apps[1].autoLaunch = true;
      browserProxy.appSettings.hasAutoLaunchApp = true;

      let items;
      return initPage()
          .then(() => {
            items = dialog.shadowRoot.querySelectorAll('.list-item');
            // Has permission to edit auto-launch so buttons should be seen.
            expectFalse(items[0].querySelector('paper-button').hidden);
            expectFalse(items[1].querySelector('paper-button').hidden);

            MockInteractions.tap(items[0].querySelector('paper-button'));
            return browserProxy.whenCalled('enableKioskAutoLaunch');
          })
          .then(appId => {
            expectEquals(appId, basicApps[0].id);

            MockInteractions.tap(items[1].querySelector('paper-button'));
            return browserProxy.whenCalled('disableKioskAutoLaunch');
          })
          .then(appId => {
            expectEquals(appId, basicApps[1].id);
          });
    });

    test(assert(TestNames.Bailout), function() {
      browserProxy.initialSettings.autoLaunchEnabled = true;
      browserProxy.appSettings.apps[1].autoLaunch = true;
      browserProxy.appSettings.hasAutoLaunchApp = true;

      expectFalse(dialog.$['confirm-dialog'].open);

      let bailoutCheckbox;
      return initPage()
          .then(() => {
            bailoutCheckbox = dialog.$$('paper-checkbox');
            // Bailout checkbox should be usable when auto-launching.
            expectFalse(bailoutCheckbox.hidden);
            expectFalse(bailoutCheckbox.disabled);
            expectFalse(bailoutCheckbox.checked);

            // Making sure canceling doesn't change anything.
            bailoutCheckbox.dispatchEvent(new PointerEvent('pointerdown'));
            Polymer.dom.flush();
            expectTrue(dialog.$['confirm-dialog'].open);

            MockInteractions.tap(
                dialog.$['confirm-dialog'].querySelector('.cancel-button'));
            Polymer.dom.flush();
            expectFalse(bailoutCheckbox.checked);
            expectFalse(dialog.$['confirm-dialog'].open);
            expectTrue(dialog.$.dialog.open);

            // Accepting confirmation dialog should trigger browserProxy call.
            bailoutCheckbox.dispatchEvent(new PointerEvent('pointerdown'));
            Polymer.dom.flush();
            expectTrue(dialog.$['confirm-dialog'].open);

            MockInteractions.tap(
                dialog.$['confirm-dialog'].querySelector('.action-button'));
            Polymer.dom.flush();
            expectTrue(bailoutCheckbox.checked);
            expectFalse(dialog.$['confirm-dialog'].open);
            expectTrue(dialog.$.dialog.open);
            return browserProxy.whenCalled('setDisableBailoutShortcut');
          })
          .then(disabled => {
            expectTrue(disabled);

            // Test clicking on checkbox again should simply re-enable bailout.
            browserProxy.reset();
            bailoutCheckbox.dispatchEvent(new PointerEvent('pointerdown'));
            expectFalse(bailoutCheckbox.checked);
            expectFalse(dialog.$['confirm-dialog'].open);
            return browserProxy.whenCalled('setDisableBailoutShortcut');
          })
          .then(disabled => {
            expectFalse(disabled);
          });
    });

    test(assert(TestNames.AddButton), function() {
      const addButton = dialog.$['add-button'];
      expectTrue(!!addButton);
      expectTrue(addButton.disabled);

      const addInput = dialog.$['add-input'];
      addInput.value = 'blah';
      expectFalse(addButton.disabled);

      MockInteractions.tap(addButton);
      return browserProxy.whenCalled('addKioskApp').then(appId => {
        expectEquals(appId, 'blah');
      });
    });

    test(assert(TestNames.Updated), function() {
      const items = dialog.shadowRoot.querySelectorAll('.list-item');
      expectTrue(items[0].textContent.indexOf(basicApps[0].name) !== -1);

      const newName = 'completely different name';

      cr.webUIListenerCallback('kiosk-app-updated', {
        id: basicApps[0].id,
        name: newName,
        iconURL: '',
        autoLaunch: false,
        isLoading: false,
      });

      expectFalse(items[0].textContent.indexOf(basicApps[0].name) !== -1);
      expectTrue(items[0].textContent.indexOf(newName) !== -1);
    });

    test(assert(TestNames.AddError), function() {
      const addInput = dialog.$['add-input'];

      expectFalse(!!addInput.invalid);
      cr.webUIListenerCallback('kiosk-app-error', basicApps[0].id);

      expectTrue(!!addInput.invalid);
      console.log(addInput.errorMessage);
      expectTrue(addInput.errorMessage.indexOf(basicApps[0].id) !== -1);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
