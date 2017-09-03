// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


(function() {
'use strict';

/* A dummy scrollbar to show the position of the line chart and to scroll the
 * line chart. */
class Scrollbar_ {
  constructor(callback) {
    this.callback_ = callback;
    this.range_ = 0;
    this.position_ = 0;
    this.width_ = 0;

    this.outerDiv_ =
        createElementWithClassName('div', 'horizontal-scrollbar-outer');
    this.outerDiv_.onscroll = this.onScroll_.bind(this);
    this.innerDiv_ =
        createElementWithClassName('div', 'horizontal-scrollbar-inner');
    this.outerDiv_.appendChild(this.innerDiv_);
  }

  onScroll_() {
    const newPosition = this.outerDiv_.scrollLeft;
    if (newPosition == this.position_)
      return;
    this.position_ = newPosition;
    this.callback_();
  }

  getRootDiv() {
    return this.outerDiv_;
  }

  getHeight() {
    return this.outerDiv_.offsetHeight;
  }

  getRange() {
    return this.range_;
  }

  getPosition() {
    return this.position_;
  }

  resize(width) {
    if (this.width_ == width)
      return;
    this.width_ = width;
    this.updateOuterDivWidth_();
  }

  updateOuterDivWidth_() {
    this.constructor.setNodeWidth(this.outerDiv_, this.width_);
  }

  setRange(range) {
    this.range_ = range;
    this.updateInnerDivWidth_();
    if (range < this.position_) {
      this.position_ = range;
      this.updateScrollbarPosition_();
    }
  }

  updateInnerDivWidth_() {
    const width = this.outerDiv_.clientWidth;
    this.constructor.setNodeWidth(this.innerDiv_, width + this.range_);
  }

  static setNodeWidth(node, width) {
    node.style.width = width + 'px';
  }

  setPosition(position) {
    if (position < 0) {
      position = 0;
    } else if (position > this.range_) {
      position = this.range_;
    }
    this.position_ = position;
    this.updateScrollbarPosition_();
  }

  updateScrollbarPosition_() {
    if (this.outerDiv_.scrollLeft == this.position_)
      return;
    this.outerDiv_.scrollLeft = this.position_;
  }

  isScrolledToRightEdge() {
    // crbug.com/760425
    return this.position_ == this.range_;
  }

  scrollToRightEdge() {
    this.setPosition(this.range_);
  }
}

/**
 * @const
 */
LineChart.Scrollbar = Scrollbar_;
})();