/* Copyright 2017 The Chromium Authors. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file. */

cr.define('safe_browsing', function() {
'use strict';

  function initialize() {
      chrome.send('expParamList', []);
      chrome.send('sbPreferencesList', []);

  }

  function addExperiment(result) {
      var resLength = result.length;
      var experimentsListFormatted = "";

      for (var i = 0; i < resLength; i += 2) {
          experimentsListFormatted += "<div><b>" + result[i + 1] +
          "</b>: " + result[i] +
              "</div>";
      }

      $('experimentsList').innerHTML = experimentsListFormatted;
  }

  function addPreferences(result) {
       var resLength = result.length;
       var preferencesListFormatted = "";

      for (var i = 0; i < resLength; i += 2) {
          preferencesListFormatted += "<div><b>" + result[i + 1] + "</b>: "
           + result[i] + "</div>";
      }

      $('preferencesList').innerHTML = preferencesListFormatted;
  }

  return {
      addExperiment: addExperiment,
      addPreferences: addPreferences,
      initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', safe_browsing.initialize);
