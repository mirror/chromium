// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Tests that an observation matches the expected value.
 * @param {Object} expected The expected value.
 * @param {Object} observed The actual value.
 * @param {string=} opt_message Optional message to include with a test
 *     failure.
 */
function assertEquals(expected, observed, opt_message) {
  if (observed !== expected) {
    var message = 'Assertion Failed\n  Observed: ' + observed +
        '\n  Expected: ' + expected;
    if (opt_message)
      message = message + '\n  ' + opt_message;
    throw new Error(message);
  }
}

/**
 * Verifies that a test result is true.
 * @param {boolean} observed The observed value.
 * @param {string=} opt_message Optional message to include with a test
 *     failure.
 */
function assertTrue(observed, opt_message) {
  assertEquals(true, observed, opt_message);
}

/**
 * Verifies that a test result is false.
 * @param {boolean} observed The observed value.
 * @param {string=} opt_message Optional message to include with a test
 *     failure.
 */
function assertFalse(observed, opt_message) {
  assertEquals(false, observed, opt_message);
}

/**
 * Verifies that the observed and reference values differ.
 * @param {Object} reference The target value for comparison.
 * @param {Object} observed The test result.
 * @param {string=} opt_message Optional message to include with a test
 *     failure.
 */
function assertNotEqual(reference, observed, opt_message) {
  if (observed === reference) {
    var message = 'Assertion Failed\n  Observed: ' + observed +
        '\n  Reference: ' + reference;
    if (opt_message)
      message = message + '\n  ' + opt_message;
    throw new Error(message);
  }
}

/**
 * Verifies that a test evaluation results in an exception.
 * @param {!Function} f The test function.
 */
function assertThrows(f) {
  var triggeredError = false;
  try {
    f();
  } catch(err) {
    triggeredError = true;
  }
  if (!triggeredError)
    throw new Error('Assertion Failed: throw expected.');
}

/**
 * Verifies that the contents of the expected and observed arrays match.
 * @param {!Array} expected The expected result.
 * @param {!Array} observed The actual result.
 */
function assertArrayEquals(expected, observed) {
  var v1 = Array.prototype.slice.call(expected);
  var v2 = Array.prototype.slice.call(observed);
  var equal = v1.length == v2.length;
  if (equal) {
    for (var i = 0; i < v1.length; i++) {
      if (v1[i] !== v2[i]) {
        equal = false;
        break;
      }
    }
  }
  if (!equal) {
    var message =
       ['Assertion Failed', 'Observed: ' + v2, 'Expected: ' + v1].join('\n  ');
    throw new Error(message);
  }
}

/**
 * Runs all functions starting with test and reports success or
 * failure of the test suite.
 */
function runTests() {
  var tests = [];
  var success = true;
  for (var name in window) {
    if (typeof window[name] == 'function' && /^test/.test(name))
      tests.push(name);
  }
  if (!tests.length) {
    console.error('\nFailed to find test cases.');
    success = false;
  }
  for (var i = 0; i < tests.length; i++) {
    try {
      if (window.setUp)
        window.setUp();
      window[tests[i]]();
    } catch (err) {
      console.error('Failure in test ' + tests[i] + '\n' + err);
      console.log(err.stack);
      success = false;
    }
    if (window.tearDown)
      window.tearDown();
  }
  endTests(success);
}

/**
 * Signals completion of a test.
 * @param {boolean} success Indicates if the test completed successfully.
 */
function endTests(success) {
  domAutomationController.setAutomationId(1);
  domAutomationController.send(success ? 'SUCCESS' : 'FAILURE');
}

window.onerror = function() {
  endTests(false);
};
