// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Class representing the client tool-bar.
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * @param {Element} toolbar The HTML element representing the tool-bar.
 * @constructor
 */
remoting.Toolbar = function(toolbar) {
  /**
   * @type {Element}
   * @private
   */
  this.toolbar_ = toolbar;
  /**
   * @type {Element}
   * @private
   */
  this.stub_ = toolbar.querySelector('.toolbar-stub');
  /**
   * @type {number?} The id of the preview timer, if any.
   * @private
   */
  this.timerId_ = null;
  /**
   * @type {number} The left edge of the tool-bar stub, updated on resize.
   * @private
   */
  this.stubLeft_ = 0;
  /**
   * @type {number} The right edge of the tool-bar stub, updated on resize.
   * @private
   */
  this.stubRight_ = 0;

  /** @type {remoting.Toolbar} */
  var that = this;

  window.addEventListener('mousemove', remoting.Toolbar.onMouseMove, false);
  window.addEventListener('resize', function() { that.center(); }, false);

  // Prevent the preview canceling if the user is interacting with the tool-bar.
  var stopTimer = function() {
    if (that.timerId_) {
      window.clearTimeout(that.timerId_);
      that.timerId_ = null;
    }
  }
  this.toolbar_.addEventListener('mousemove', stopTimer, false);
};

/**
 * Preview the tool-bar functionality by showing it for 3s.
 * @return {void} Nothing.
 */
remoting.Toolbar.prototype.preview = function() {
  addClass(this.toolbar_, remoting.Toolbar.VISIBLE_CLASS_);
  if (this.timerId_) {
    window.clearTimeout(this.timerId_);
    this.timerId_ = null;
  }
  /** @type {remoting.Toolbar} */
  var that = this;
  var endPreview = function() {
    removeClass(that.toolbar_, remoting.Toolbar.VISIBLE_CLASS_);
  };
  this.timerId_ = window.setTimeout(endPreview, 3000);
};

/**
 * Center the tool-bar horizonally.
 */
remoting.Toolbar.prototype.center = function() {
  var toolbarX = (window.innerWidth - this.toolbar_.clientWidth) / 2;
  this.toolbar_.style['left'] = toolbarX + 'px';
  var r = this.stub_.getBoundingClientRect();
  this.stubLeft_ = r.left;
  this.stubRight_ = r.right;
};

/**
 * Toggle the tool-bar visibility.
 */
remoting.Toolbar.prototype.toggle = function() {
  if (hasClass(this.toolbar_.className,
               remoting.Toolbar.VISIBLE_CLASS_)) {
    removeClass(this.toolbar_, remoting.Toolbar.VISIBLE_CLASS_);
  } else {
    addClass(this.toolbar_, remoting.Toolbar.VISIBLE_CLASS_);
  }
};

/**
 * Test the specified co-ordinate to see if it is close enough to the stub
 * to activate it.
 *
 * @param {number} x The x co-ordinate.
 * @param {number} y The y co-ordinate.
 * @return {boolean} True if the position should activate the tool-bar stub, or
 *     false otherwise.
 * @private
 */
remoting.Toolbar.prototype.hitTest_ = function(x, y) {
  var threshold = 50;
  return (x >= this.stubLeft_ - threshold &&
          x <= this.stubRight_ + threshold &&
          y < threshold);
};

/**
 * Called whenever the mouse moves in the document. This is used to make the
 * active area of the tool-bar stub larger without making a corresponding area
 * of the host screen inactive.
 *
 * @param {Event} event The mouse move event.
 * @return {void} Nothing.
 */
remoting.Toolbar.onMouseMove = function(event) {
  if (remoting.toolbar) {
    if (remoting.toolbar.hitTest_(event.x, event.y)) {
      addClass(remoting.toolbar.stub_, remoting.Toolbar.STUB_EXTENDED_CLASS_);
    } else {
      removeClass(remoting.toolbar.stub_,
                  remoting.Toolbar.STUB_EXTENDED_CLASS_);
    }
  } else {
    document.removeEventListener('mousemove',
                                 remoting.Toolbar.onMouseMove, false);
  }
};

/** @type {remoting.Toolbar} */
remoting.toolbar = null;

/** @private */
remoting.Toolbar.STUB_EXTENDED_CLASS_ = 'toolbar-stub-extended';
/** @private */
remoting.Toolbar.VISIBLE_CLASS_ = 'toolbar-visible';
