// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview CrContainerShadowBehavior holds logic for showing a drop shadow
 * near the top of a container element, when the content has scrolled.
 *
 * Elements using this behavior are expected to define the following three DOM
 * nodes:
 *  - #container: The element being scrolled.
 *  - #dropShadow: The element holding the drop shadow itself.
 *  - #intersectionProbe: The element used to determine when to show the shadow.
 *
 *  A 'has-shadow' attribute is automatically added/removed to #dropShadow by
 *  this behavior while scrolling, as necessary. Clients should use that
 *  attribute to show/hide/animate the drop shadow.
 */

/** @polymerBehavior */
var CrContainerShadowBehavior = {
  /** @private {?IntersectionObserver} */
  intersectionObserver_: null,

  /** @override */
  attached: function() {
    // Setup drop shadow logic.
    var callback = entries => {
      this.$.dropShadow.classList.toggle(
          'has-shadow', entries[entries.length - 1].intersectionRatio == 0);
    };

    this.intersectionObserver_ = new IntersectionObserver(
        callback,
        /** @type {IntersectionObserverInit} */ ({
          root: this.$.container,
          threshold: 0,
        }));
    this.intersectionObserver_.observe(this.$.intersectionProbe);
  },

  /** @override */
  detached: function() {
    this.intersectionObserver_.disconnect();
    this.intersectionObserver_ = null;
  },
};
