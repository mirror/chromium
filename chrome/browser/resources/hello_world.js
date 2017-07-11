// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('hello_world', function() {
  'use strict';

  /**
   * Be polite and insert translated hello world strings for the user
   * on loading. */
  function initialize() {
    $('welcome-message').textContent = loadTimeData.getStringF(
        'welcomeMessage', loadTimeData.getString('userName'));
  }

  // Return an object with all of the exports.
  return {
    initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', hello_world.initialize);
