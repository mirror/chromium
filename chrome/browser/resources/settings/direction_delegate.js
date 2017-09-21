// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('settings');

cr.define('settings', function() {
  /** @interface */
  class DirectionDelegate {
    /**
     * @return {boolean} Whether the direction of the settings UI is
     *     right-to-left.
     */
    isRtl() {}
  }

  /** @implements {settings.DirectionDelegate} */
  class DirectionDelegateImpl {
    /** @override */
    isRtl() {
      return loadTimeData.getString('textdirection') == 'rtl';
    }
  }

  // A singleton instance to be shared among any UI elements that need RTL/LTR
  // information.
  var instance = null;

  function getDirectionDelegate() {
    if (instance === null)
      instance = new DirectionDelegateImpl();

    return instance;
  }

  /** @param {!settings.DirectionDelegate} delegate */
  function setDirectionDelegateForTesting(delegate) {
    instance = delegate;
  }

  return {
    DirectionDelegate: DirectionDelegate,
    getDirectionDelegate: getDirectionDelegate,
    setDirectionDelegateForTesting: setDirectionDelegateForTesting,
  };
});
