// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('interventions_internals', function() {
  let pageHandler_ = null;

  function init(pageHandler) {
    pageHandler_ = pageHandler;
  }

  function getPreviewsEnabled() {
    if (pageHandler_.getPreviewsEnabled == null) {
      return;
    }
    let node = document.createElement('p');
    $('log-message').appendChild(node);
    return pageHandler_.getPreviewsEnabled()
        .then(function(response) {
          let message = 'Previews enabled: ';
          message += response.enabled ? 'Enabled' : 'Disabled';
          node.textContent = message;
        })
        .catch(function(error) {
          node.textContent = error.message;
        });
  }

  return {
    init: init,
    getPreviewsEnabled: getPreviewsEnabled,
  };
});

window.setupFn = window.setupFn || function() {
  return Promise.resolve();
};

document.addEventListener('DOMContentLoaded', function() {
  let pageHandler = null;

  window.setupFn().then(function() {
    if (window.testPageHandler) {
      pageHandler = window.testPageHandler;
    } else {
      pageHandler = new mojom.InterventionsInternalsPageHandlerPtr;
      Mojo.bindInterface(
          mojom.InterventionsInternalsPageHandler.name,
          mojo.makeRequest(pageHandler).handle);
    }
    pageHandler.finishJob = pageHandler.finishJob || function() {};
    interventions_internals.init(pageHandler);
    interventions_internals.getPreviewsEnabled();
    pageHandler.finishJob();
  });
});
