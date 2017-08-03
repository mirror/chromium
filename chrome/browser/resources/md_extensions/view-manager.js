// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  var ViewManager = Polymer({
    is: 'extensions-view-manager',

    properties: {
      selected: String
    },

    observers: [
      'onSelectedChanged_(selected)'
    ],

    onSelectedChanged_: function() {
      var prevPage = this.querySelector('.active');
      var newPage = this.querySelector('#' + this.selected);

      this.exit_(prevPage);
      this.enter_(newPage);
    },

    exit_: function(prevPage) {
      if(prevPage) {
        prevPage.addEventListener('transitionend', function f(){
          prevPage.classList.remove('block');
          prevPage.removeEventListener('transitionend', f);
        });

        prevPage.classList.add('block');
        prevPage.classList.remove('active');
      }
    },

    enter_: function(newPage) {
      assert(newPage);
      newPage.classList.add('block');
      newPage.scrollTop;
      newPage.classList.add('active');
    },
  });

  return {ViewManager: ViewManager};
});