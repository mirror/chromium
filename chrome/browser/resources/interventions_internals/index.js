// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
// Connection to the InterventionsInternalsPageHandler instance running in the
// browser process.
let pageHandler = null;

function getPreviewsEnabled() {
  let node = document.createElement('p');

  pageHandler.getPreviewsEnabled()
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
}

document.addEventListener('DOMContentLoaded', function() {
  pageHandler = new mojom.InterventionsInternalsPageHandlerPtr;
  Mojo.bindInterface(
      mojom.InterventionsInternalsPageHandler.name,
      mojo.makeRequest(pageHandler).handle);
  getPreviewsEnabled();
});
})();
