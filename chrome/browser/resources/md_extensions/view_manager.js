// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  /**
   * TODO(scottchen): shim for not having Animation.finished implemented. Can
   * replace with Animation.finished if Chrome implements it.
   * @param {Animation} animation
   * @return {Promise}
   */
  function whenFinished(animation) {
    return new Promise(function(resolve, reject) {
      animation.addEventListener('finish', resolve);
    });
  }

  /** @type {Map<string, function(Element): Promise>} */
  var viewAnimations = new Map();
  viewAnimations.set('no-animation', element => {
    return Promise.resolve();
  });
  viewAnimations.set('fade-in', element => {
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

    return whenFinished(anim);
  });
  viewAnimations.set('fade-out', element => {
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

    return whenFinished(anim);
  });

  var ViewManager = Polymer({
    is: 'extensions-view-manager',

    /**
     * @private
     * @param {Element} element
     * @param {string} animation
     * @return {Promise}
     */
    exit_: function(element, animation) {
      assert(element);
      var animationFunction = extensions.viewAnimations.get(animation);
      assert(animationFunction);

      element.fire('view-exit-start');
      return animationFunction(element).then(function() {
        element.classList.remove('active');
        element.fire('view-exit-finish');
      });
    },

    /**
     * @private
     * @param {Element} element
     * @param {string} animation
     * @return {Promise}
     */
    enter_: function(element, animation) {
      assert(element);
      var animationFunction = extensions.viewAnimations.get(animation);
      assert(animationFunction);

      element.classList.add('active');
      element.fire('view-enter-start');
      return animationFunction(element).then(function() {
        element.fire('view-enter-finish');
      });
    },

    /**
     * @param {string} newViewId
     * @param {string=} enterAnimation
     * @param {string=} exitAnimation
     * @return {Promise}
     */
    switchView: function(newViewId, enterAnimation, exitAnimation) {
      var previousView = this.querySelector('.active');
      var newView = this.querySelector('#' + newViewId);

      var promises = [];
      if (previousView) {
        promises.push(this.exit_(
            previousView, exitAnimation ? exitAnimation : 'fade-out'));
        promises.push(
            this.enter_(newView, enterAnimation ? enterAnimation : 'fade-in'));
      } else {
        promises.push(this.enter_(newView, 'no-animation'));
      }

      return Promise.all(promises);
    },
  });

  return {viewAnimations: viewAnimations, ViewManager: ViewManager};
});