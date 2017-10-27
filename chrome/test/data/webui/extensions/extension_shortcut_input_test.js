// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-keyboard-shortcuts. */
cr.define('extension_shortcut_input_tests', function() {
  class DelegateMock {
    constructor() {
      // For test verification only.
      this.binding = {};
      this.startCaptureFlag = false;
    }

    /**
     * @param {boolean} enable
     */
    setShortcutHandlingSuspended(enable) {
      this.startCaptureFlag = enable;
    }

    /**
     * @param {string} item
     * @param {string} commandName
     * @param {string} keybinding
     */
    updateExtensionCommand(item, commandName, keybinding) {
      this.binding.item = item;
      this.binding.commandName = commandName;
      this.binding.keybinding = keybinding;
    }

    /* Testing only. */
    verifyExtensionCommand(binding) {
      expectEquals(this.binding.item, binding.item);
      expectEquals(this.binding.commandName, binding.commandName);
      expectEquals(this.binding.keybinding, binding.keybinding);
    }
  }

  /** @enum {string} */
  var TestNames = {
    Basic: 'basic',
  };

  suite('ExtensionShortcutInputTest', function() {
    /** @type {extensions.ShortcutInput} */
    var input;
    setup(function() {
      PolymerTest.clearBody();
      input = new extensions.ShortcutInput();
      input.delegate = new DelegateMock();
      input.commandName = 'Command';
      input.item = 'itemid';
      document.body.appendChild(input);
      Polymer.dom.flush();
    });

    test(assert(TestNames.Basic), function() {
      var field = input.$['input'];
      var fieldText = function() {
        return field.value;
      };
      expectEquals('', fieldText());
      var isClearVisible =
          extension_test_util.isVisible.bind(null, input, '#clear', false);
      expectFalse(isClearVisible());

      // Click the input. Capture should start.
      expectFalse(input.delegate.startCaptureFlag);
      MockInteractions.tap(field);
      expectTrue(input.delegate.startCaptureFlag);

      expectEquals('', fieldText());
      expectTrue(input.capturing_);
      expectFalse(isClearVisible());

      // Press ctrl.
      MockInteractions.keyDownOn(field, 17, ['ctrl']);
      expectEquals('Ctrl', fieldText());
      expectTrue(input.capturing_);
      // Add shift.
      MockInteractions.keyDownOn(field, 16, ['ctrl', 'shift']);
      expectEquals('Ctrl + Shift', fieldText());
      expectTrue(input.capturing_);
      // Remove shift.
      MockInteractions.keyUpOn(field, 16, ['ctrl']);
      expectEquals('Ctrl', fieldText());
      // Add alt (ctrl + alt is invalid).
      MockInteractions.keyDownOn(field, 18, ['ctrl', 'alt']);
      expectEquals('invalid', fieldText());
      expectTrue(input.capturing_);
      // Remove alt.
      MockInteractions.keyUpOn(field, 18, ['ctrl']);
      expectEquals('Ctrl', fieldText());
      expectTrue(input.capturing_);

      // Add 'A'. Once a valid shortcut is typed (like Ctrl + A), it is
      // committed.
      MockInteractions.keyDownOn(field, 65, ['ctrl']);
      input.delegate.verifyExtensionCommand({
        keybinding: 'Ctrl+A',
        item: 'itemid',
        commandName: 'Command',
      });

      expectEquals('Ctrl + A', fieldText());
      expectFalse(input.capturing_);
      expectEquals('Ctrl+A', input.shortcut);
      expectTrue(isClearVisible());

      // Test clearing the shortcut.
      MockInteractions.tap(input.$['clear']);
      input.delegate.verifyExtensionCommand({
        keybinding: '',
        item: 'itemid',
        commandName: 'Command',
      });

      expectEquals('', input.shortcut);
      expectFalse(isClearVisible());

      MockInteractions.tap(field);

      // Test ending capture using the escape key.
      expectTrue(input.capturing_);
      expectTrue(input.delegate.startCaptureFlag);
      MockInteractions.keyDownOn(field, 27);  // Escape key.
      expectFalse(input.capturing_);
      expectFalse(input.delegate.startCaptureFlag);
    });
  });

  return {
    TestNames: TestNames,
  };
});
