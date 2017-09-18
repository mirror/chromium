// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
// Connection to the InterventionsInternalsPageHandler instance running in the
// browser process.
let pageHandler = null;

console.log("HERE");

function getLogMessage() {
  pageHandler
      .logMessage()
      .then(function(response) {
        if (response.success) {
          console.log("SUCCESS!");
          let node = document.createElement('p');
          let message = document.createTextNode(response.message);
          node.appendChild(message);

          $('log-message').appendChild(node);
        }
      })
      .catch(function(error) {
        console.log(error.message);
      });
}

document.addEventListener('DOMContentLoaded', function() {
  pageHandler = new mojom.InterventionsInternalsPageHandlerPtr;
  Mojo.bindInterface(
      mojom.InterventionsInternalsPageHandler.name, mojo.makeRequest(pageHandler).handle);
  getLogMessage();
});
})();
