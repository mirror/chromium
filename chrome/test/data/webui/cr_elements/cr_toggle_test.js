// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('cr-toggle', function() {
  var toggle;

  setup(function() {
    PolymerTest.clearBody();
    document.body.innerHTML = `
      <cr-toggle id="toggle"></cr-toggle>
    `;

    toggle = document.getElementById('toggle');
    assertNotChecked();
  });

  function assertChecked() {
    assertTrue(toggle.checked);
    assertTrue(toggle.hasAttribute('checked'));
    assertEquals('true', toggle.getAttribute('aria-pressed'));
    // Asserting that the toggle button has actually moved.
    assertTrue(getComputedStyle(toggle.$.button).transform.includes('matrix'));
  }

  function assertNotChecked() {
    assertFalse(toggle.checked);
    assertEquals(null, toggle.getAttribute('checked'));
    assertEquals('false', toggle.getAttribute('aria-pressed'));
    // Asserting that the toggle button has not moved.
    assertEquals('none', getComputedStyle(toggle.$.button).transform);
  }

  function assertDisabled() {
    assertTrue(toggle.disabled);
    assertTrue(toggle.hasAttribute('disabled'));
    assertEquals('true', toggle.getAttribute('aria-disabled'));
  }

  function assertNotDisabled() {
    assertFalse(toggle.disabled);
    assertFalse(toggle.hasAttribute('disabled'));
    assertEquals('false', toggle.getAttribute('aria-disabled'));
  }

  test('ToggleByAttribute', function() {
    test_util.eventToPromise('change', toggle).then(function() {
      // Should not fire 'change' event when state is changed programmatically.
      // Only user interaction should result in 'change' event.
      assertFalse(true);
    });

    toggle.checked = true;
    assertChecked();

    toggle.checked = false;
    assertNotChecked();
  });

  test('ToggleByPointer', function() {
    var whenChanged = test_util.eventToPromise('change', toggle);
    MockInteractions.tap(toggle);
    return whenChanged.then(function() {
      assertChecked();
      whenChanged = test_util.eventToPromise('change', toggle);
      MockInteractions.tap(toggle);
    }).then(function() {
      assertNotChecked();
    });
  });

  test('ToggleByKey', function() {
    var whenChanged = test_util.eventToPromise('change', toggle);
    MockInteractions.keyEventOn(toggle, 'keypress', 13, undefined, 'Enter');
    return whenChanged.then(function() {
      assertChecked();
      whenChanged = test_util.eventToPromise('change', toggle);
      MockInteractions.keyEventOn(toggle, 'keypress', 32, undefined, 'Space');
    }).then(function() {
      assertNotChecked();
    });
  });

  test('ToggleWhenDisabled', function() {
    assertNotDisabled();
    toggle.disabled = true;
    assertDisabled();

    MockInteractions.tap(toggle);
    assertNotChecked();
    assertDisabled();

    toggle.disabled = false;
    MockInteractions.tap(toggle);
    assertChecked();
  });
});
