// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A base class for all test browser proxies to inherit from. Provides helper
 * methods for allowing tests to track when a method was called.
 *
 * Subclasses are responsible for calling |methodCalled|, when a method is
 * called, which will trigger callers of |whenCalled| to get notified.
 * For example:
 * --------------------------------------------------------------------------
 * class MyTestBrowserProxy extends TestBrowserProxy {
 *   constructor() {
 *     super(['myMethod']);
 *   }
 *
 *   myMethod(someId) {
 *     this.methodCalled('myMethod', someId);
 *   }
 * };
 *
 * // Test code sample
 *
 * var testBrowserProxy = new MyTestBrowserProxy();
 * // ...Replacing real proxy with test proxy....
 * simulateClickFooButton();
 * testBrowserProxy.whenCalled('fooMethod').then(function(id) {
 *   assertEquals(EXPECTED_ID, id);
 * });
 * --------------------------------------------------------------------------
 */
class TestBrowserProxy {
  /**
   * @param {!Array<string>} methodNames Names of all methods whose calls
   *     need to be tracked.
   */
  constructor(methodNames) {
    /** @private {!Map<string, !PromiseResolver>} */
    this.resolverMap_ = new Map();
    methodNames.forEach((methodName) => {
      this.resolverMap_.set(methodName, new PromiseResolver());
    }, this);
  }

  /**
   * Called by subclasses when a tracked method is called from the code that
   * is being tested.
   * @param {string} methodName
   * @param {*=} opt_arg Optional argument to be forwarded to the testing
   *     code, useful for checking whether the proxy method was called with
   *     the expected arguments.
   * @protected
   */
  methodCalled(methodName, opt_arg) {
    let method = this.resolverMap_.get(methodName);
    if (method) {
      method.resolve(opt_arg);
    } else {
      // Tip: check that the |methodName| is being passed to |this.constructor|.
      throw 'Method name not found in methodCalled(\'' + methodName +
          '\', ...);';
    }
  }

  /**
   * @param {string} methodName
   * @return {!Promise} A promise that is resolved when the given method
   *     is called.
   */
  whenCalled(methodName) {
    let method = this.resolverMap_.get(methodName);
    if (method)
      return method.promise;
    // Tip: check that the |methodName| is being passed to |this.constructor|.
    throw 'Method name not found in whenCalled(\'' + methodName + '\');';
  }

  /**
   * Resets the PromiseResolver associated with the given method.
   * @param {string} methodName
   */
  resetResolver(methodName) {
    let method = this.resolverMap_.get(methodName);
    if (!method) {
      // Tip: check that the |methodName| is being passed to |this.constructor|.
      throw 'Method name not found in resetResolver(\'' + methodName + '\');';
    }
    this.resolverMap_.set(methodName, new PromiseResolver());
  }

  /**
   * Resets all PromiseResolvers.
   */
  reset() {
    this.resolverMap_.forEach(function(value, methodName) {
      this.resolverMap_.set(methodName, new PromiseResolver());
    }.bind(this));
  }
}
