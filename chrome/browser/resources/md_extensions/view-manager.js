// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  // TODO(scottchen): shim for not having Animation.finished implemented. Can
  // replace with Animation.finished if Chrome implements it.
  var finished = function(animation) {
    return new Promise(function(resolve, reject) {
      animation.addEventListener('finish', resolve);
    });
  };

  var viewAnimations = {
    'no-animation': function(element) {
      return Promise.resolve();
    },
    'fade-in': function(element) {
      var anim = element.animate(
          [
            {opacity: 0},
            {opacity: 1},
          ],
          /** @type {!KeyframeEffectOptions} */ ({
            duration: 180,
            easing: 'ease-in-out',
            iterations: 1,
          }));

      return finished(anim);
    },
    'fade-out': function(element) {
      var anim = element.animate(
          [
            {opacity: 1},
            {opacity: 0},
          ],
          /** @type {!KeyframeEffectOptions} */ ({
            duration: 180,
            easing: 'ease-in-out',
            iterations: 1,
          }));

      return finished(anim);
    },
  };

  var ViewManager = Polymer({
    is: 'extensions-view-manager',

    properties: {selected: String},

    observers: ['onSelectedChanged_(selected)'],

    onSelectedChanged_: function() {
      var prevPage = this.querySelector('.active');
      var newPage = this.querySelector('#' + this.selected);

      // TODO(scottchen): make animation type configurable.
      if(prevPage) {
        this.exit_(prevPage, 'fade-out');
        this.enter_(newPage, 'fade-in');
      } else {
        this.enter_(newPage, 'no-animation');
      }
    },

    exit_: function(prevPage, animation) {
      assert(prevPage);
      extensions.viewAnimations[animation](prevPage).then(function() {
        prevPage.classList.remove('active');
      });
    },

    enter_: function(newPage, animation) {
      assert(newPage);
      newPage.classList.add('active');
      newPage.scrollTop;
      extensions.viewAnimations[animation](newPage).then(function() {
        //In case newPage contains iron-list, fire resize to render correctly.
        newPage.fire('resize');
      });
    },
  });

  return {viewAnimations: viewAnimations, ViewManager: ViewManager};
});