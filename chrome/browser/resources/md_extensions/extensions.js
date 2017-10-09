// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

// Going to any unsupported routes should redirect to chrome://extensions/
// TODO(scottchen): there might be a better place to do this once manager
//     and service become less coupled.
if (!extensions.navigation.isRouteSupported()) {
  location.pathname = '/';
  return;
}

const manager = /** @type {extensions.Manager} */ (
    document.querySelector('extensions-manager'));
manager.readyPromiseResolver.promise.then(function() {
  extensions.Service.getInstance().managerReady(manager);
});
})();
