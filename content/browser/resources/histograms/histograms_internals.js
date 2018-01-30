// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Initiates the request for histograms.
 */
function requestHistograms() {
  var query = '';
  if (document.location.pathname)
    query = document.location.pathname.substring(1);
  cr.sendWithPromise('requestHistograms', query).then((histograms) =>
        addHistograms(histograms));
}

/**
 * Callback from backend with the list of histograms. Builds the UI.
 * @param {array} histograms The list of histograms.
 */
function addHistograms(histograms) {
  $('histograms').innerHTML = '';
  for (let histogram of histograms) {
    var div = document.createElement('div');
    div.appendChild(document.createTextNode(JSON.stringify(histogram)));
    $("histograms").appendChild(div);
  }
}

/**
 * Load the initial list of histograms.
 */
document.addEventListener('DOMContentLoaded', function() {
  $('refresh').onclick = function() {
    requestHistograms();
    return false;
  };

  requestHistograms();
});
