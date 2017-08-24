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
function setUpSimpleTests(templateId) {
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
  if (!condition) {
    throw new Error(opt_message || 'Assertion failed');
  }
}


/**
 * Aborts a test if |expected| !== |got|.
 * @param {object} expected The expected value.
 * @param {object} got The obtained value.
 * @param {string=} opt_message A message to log if the paremeters
 *    are not equal.
 */
function assertEquals(expected, got, opt_message) {
  if (expected !== got) {
    throw new Error(opt_message || `Expected: '${expected}', got: '${got}'`);
  }
}


function assertThrows(expected, throws, opt_message) {
  try {
    throws.call();
  } catch (err) {
    if (err.message.includes(expected)) {
      return;
    }
    throw new Error(opt_message || `Expected: '${expected}', got: '${err}'`);
  }
  throw new Error(opt_message || 'Got no error.');
}


/**
 * Runs all simple tests, i.e. those that don't require interaction from the
 * native side.
 * @return {boolean} True if all tests pass and false otherwise.
 */
function runSimpleTests() {
  let pass = true;
  let totalTests = 0;
  let passedTests = 0;
  for (var testName in window) {
    if (/^test.+/.test(testName) && typeof window[testName] == 'function') {
      totalTests++;
      let testPassed = false;
      try {
        testPassed = runTest(testName);
      } catch (err) {
        console.log('Error occurred while running tests, aborting.');
        return false;
      }
      pass = pass && testPassed;
      if (testPassed) {
        passedTests++;
      }
    }
  }
  window.console.log(`Passed tests: ${passedTests}/${totalTests}`);
  return pass;
}

function runTest(testName) {
  function runTest_(testName) {
    try {
      window[testName].call(window);
    } catch (err) {
      window.console.log(`Test '${testName}' failed: ${err}`);
      window.console.log('Stacktrace:\n' + err.stack);
      return false;
    }
    return true;
  }

  let testPassed = false;
  try {
     try {
      if (typeof window.setUp === 'function') {
        window.setUp();
      }
    } catch (err) {
      window.console.log(`Setup for '${testName}' failed: ${err}`);
      window.console.log('Stacktrace:\n' + err.stack);
      throw new Error('Setup failed, aborting tests.');
    }
    testPassed = runTest_(testName);
  } finally {
    try {
      if (typeof window.tearDown === 'function') {
        window.tearDown();
      }
    } catch (err) {
      window.console.log(`Tear down for '${testName}' failed: ${err}`);
      window.console.log('Stacktrace:\n' + err.stack);
      throw new Error('Tear down failed, aborting tests.');
    }
  }
  return testPassed;
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

function Replacer() {
  this.old_ = {};


  this.replace = function(parent, propertyName, newValue) {
    if (!(propertyName in parent)) {
      throw new Error('Cannot replace missing property "' + propertyName +
          '" in ' + parent);
    }
    this.old_[propertyName] = {};
    this.old_[propertyName].value = parent[propertyName];
    this.old_[propertyName].parent = parent;
    parent[propertyName] = newValue;
  };


  this.reset = function() {
    Object.entries(this.old_).forEach(
    ([propertyName, saved]) => {
       saved.parent[propertyName] = saved.value;
    });
  };
}

function MockClock() {
  this.time = 0;


  /**
   * Records the timeouts that have been set and not yet executed, as an array
   * of {callback, activationTime, id} objects, ordered by activationTime.
   */
  this.pendingTimeouts = [];


  this.nextTimeoutId = 0;


  this.stubs_ = new Replacer();


  /**
   * Sets the current time that sbox.getTime returns;
   * @param {number} msec The time in milliseconds.
   */
  this.setTime = function(msec) {
    assert(this.time <= msec);
    this.time = msec;
  };


  /**
   * Advances the current time by the specified number of milliseconds.
   * @param {number} msec The length of time to advance the current time by.
   */
  this.advanceTime = function(msec) {
    assert(msec >= 0);
    this.time += msec;
  };


  this.dispose = function() {
    stubs_.reset();
  };


  this.isTimeoutSet = function(id) {
    return id < this.nextTimeoutId &&
      this.pendingTimeouts.map(t => t.id).includes(id);
  };


  this.install_ = function() {
    this.stubs_.replace(window, 'clearTimeout', (id) => {
      if (!id) {
        return;
      }
      for (let i = 0; i < this.pendingTimeouts.length; ++i) {
        if (this.pendingTimeouts[i].id == id) {
          this.pendingTimeouts.splice(i, 1);
          return;
        }
      }
    });
    this.stubs_.replace(window, 'setTimeout', (callback, interval) => {
      const timeoutId = ++this.nextTimeoutId;
      this.pendingTimeouts.push({
        'callback': callback,
        'activationTime': this.time + interval,
        'id': timeoutId
      });
      this.pendingTimeouts.sort(function(a, b) {
        return a.activationTime - b.activationTime;
      });
      return timeoutId;
    });
  };


  this.install_();
}