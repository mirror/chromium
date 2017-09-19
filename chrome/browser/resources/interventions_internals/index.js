// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
cr.define('interventions_internals', function() {
  let pageHandler_ = null;

  function init(pageHandler) {
    pageHandler_ = pageHandler;
  }

  function getPreviewsEnabled() {
    if (pageHandler_.getPreviewsEnabled == null) {
      return;
    }
    return new Promise(function(resolve, reject) {
      let node = document.createElement('p');
      pageHandler_.getPreviewsEnabled()
          .then(function(response) {
            let message = 'Previews enabled: ';

            if (response.enabled) {
              message += 'Enabled';
            } else {
              message += 'Disabled';
            }

            $('log-message').appendChild(node);
            node.appendChild(document.createTextNode(message));
          })
          .catch(function(error) {
            let message = document.createTextNode(error.message);
            $('log-message').appendChild(node);
            node.appendChild(message);
          });
      resolve();
    });
  }

  return {
    init: init,
    getPreviewsEnabled: getPreviewsEnabled,
  };
});

document.addEventListener('DOMContentLoaded', function() {
  let pageHandler = null;
  if (window.testPageHandler) {
    pageHandler = window.testPageHandler;
  } else {
    pageHandler = new mojom.InterventionsInternalsPageHandlerPtr;
    Mojo.bindInterface(
        mojom.InterventionsInternalsPageHandler.name,
        mojo.makeRequest(pageHandler).handle);
  }
  interventions_internals.init(pageHandler);
  interventions_internals.getPreviewsEnabled();
});
})();
