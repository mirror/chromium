// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Shortcut for document.getElementById.
 * @param {string} id of the element.
 * @return {HTMLElement} with the id.
 */
function $(id) {
  return document.getElementById(id);
}


/**
 * Sets up for the next test case. Recreates the default local NTP DOM.
 * @param {string} templateId ID of the element to clone into the body element.
 */
function setUp(templateId) {
  // First, clear up the DOM and state left over from any previous test case.
  document.body.innerHTML = '';
  // The NTP stores some state such as fakebox focus in the body's classList.
  document.body.classList = '';

  document.body.appendChild($(templateId).content.cloneNode(true));
}


/**
 * Aborts a test if a condition is not met.
 * @param {boolean} condition The condition that must be true.
 * @param {string=} opt_message A message to log if the condition is not met.
 */
function assert(condition, opt_message) {
  if (!condition)
    throw new Error(opt_message || 'Assertion failed');
}


/**
 * Runs all simple tests, i.e. those that don't require interaction from the
 * native side.
 * @param {string} templateId ID of the element to clone into the body element
 *    as part of setup.
 * @return {boolean} True if all tests pass and false otherwise.
 */
function runSimpleTests(templateId) {
  var pass = true;
  for (var testName in window) {
    if (/^test.+/.test(testName) && typeof window[testName] == 'function') {
      try {
        setUp(templateId);
      } catch (err) {
        window.console.log(`Setup for '${testName}' failed: ${err}`);
        window.console.log('Stacktrace:\n' + err.stack);
        pass = false;
      }
      try {
        window[testName].call(window);
      } catch (err) {
        window.console.log(`Test '${testName}' failed: ${err}`);
        window.console.log('Stacktrace:\n' + err.stack);
        pass = false;
      }
    }
  }
  return pass;
}


/**
 * Creates and initializes a LocalNTP object.
 * @param {boolean} isGooglePage Whether to make it a Google-branded NTP.
 */
function initLocalNTP(isGooglePage) {
  configData.isGooglePage = isGooglePage;
  var localNTP = LocalNTP();
  localNTP.init();
}


/**
 * Checks whether a given HTMLElement exists and is visible.
 * @param {HTMLElement|undefined} elem An HTMLElement.
 * @return {boolean} True if the element exists and is visible.
 */
function elementIsVisible(elem) {
  return elem && elem.offsetWidth > 0 && elem.offsetHeight > 0 &&
      window.getComputedStyle(elem).visibility != 'hidden';
}
