// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('welcome', function() {
  'use strict';

  Polymer({
    is: 'welcome-content',

    onAccept_: function() {
      chrome.send('handleActivateSignIn');
    },

    onDecline_: function(e) {
      chrome.send('handleUserDecline');
      e.preventDefault();
    },

    onLogoTap_: function() {
      this.$.logoIcon.animate(
          {
            transform: ['none', 'rotate(-10turn)'],
          },
          /** @type {!KeyframeEffectOptions} */ ({
            duration: 500,
            easing: 'cubic-bezier(1, 0, 0, 1)',
          }));
    },
  });
});
