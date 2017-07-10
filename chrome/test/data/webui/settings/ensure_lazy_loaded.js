// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings', function() {
  function ensureLazyLoaded() {
    // Only trigger lazy loading, if we are in top-level Settings page.
    if (location.href == location.origin + '/') {
      suiteSetup(function() {
        return new Promise(function(resolve, reject) {
          console.log('ensureLazyLoaded did a thing');
          // This URL needs to match the URL passed to <settings-idle-load> from
          // <settings-basic-page>.
          Polymer.Base.importHref('/lazy_load.html', resolve, reject, true);
        });
      });
    } else {
      console.log('ensureLazyLoaded called, but doing nothing');
    }
  }

  return {
    ensureLazyLoaded: ensureLazyLoaded,
  };
});
