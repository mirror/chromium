// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

class Label_ {
  constructor(units, unitScale) {
    this.units_ = units;
    this.unitScale_ = unitScale;
    this.maxValue_ = 0;
    this.currentUnit_ = 0;
    this.labels_ = [];
    this.height_ = 0;
    this.fontHeight_ = 10;  // TODO: find right value
    this.precision_ = 1;
    this.topLabelValue_ = 0;
    this.scale_ = 1;

    this.isCache_ = false;
  }

  static getSuitalbeUnit(value, units, unitScale) {
    let unitIdx = 0;
    while (unitIdx + 1 < units.length && value >= unitScale) {
      value /= unitScale;
      ++unitIdx;
    }
    return [unitIdx, value];
  }

  setMaxValue(maxValue) {
    if (this.maxValue_ == maxValue)
      return;

    const units = this.units_;
    const unitScale = this.unitScale_;

    let unit;
    [unit, maxValue] =
        this.constructor.getSuitalbeUnit(maxValue, units, unitScale);
    this.currentUnit_ = unit;
    this.maxValue_ = maxValue;
    this.isCache_ = false;
  }

  setLayout(height, fontHeight, precision) {
    if (this.height_ == height && this.fontHeight_ == fontHeight &&
        this.precision_ == precision)
      return;

    this.height_ = height;
    this.fontHeight_ = fontHeight;
    this.precision_ = precision;
    this.isCache_ = false;
  }

  updateLabels_() {
    if (this.isCache_)
      return;

    const maxValue = this.maxValue_;
    this.labels_ = [];
    if (maxValue == 0)
      return;

    // The maximum number of equally spaced labels allowed.  |fontHeight_|
    // is doubled because the top two labels are both drawn in the same
    // gap.
    const minlabelSpacing =
        2 * this.fontHeight_ + LineChart.MIN_LABEL_VERTICAL_SPACING;
    let maxLabelNum = 1 + this.height_ / minlabelSpacing;
    if (maxLabelNum < 2) {
      maxLabelNum = 2;
    } else if (maxLabelNum > LineChart.MAX_VERTICAL_LABEL_NUM) {
      maxLabelNum = LineChart.MAX_VERTICAL_LABEL_NUM;
    }

    const precision = this.precision_;

    // Initial try for step size between conecutive labels.
    let stepSize = Math.pow(10, -precision);
    // Number of digits to the right of the decimal of |stepSize|.
    // Used for formating label strings.
    let stepSizePrecision = precision;

    // Pick a reasonable step size.
    while (true) {
      // If we use a step size of |stepSize| between labels, we'll need:
      //
      // Math.ceil(maxValue / stepSize) + 1
      //
      // labels.  The + 1 is because we need labels at both at 0 and at
      // the top of the graph.

      // Check if we can use steps of size |stepSize|.
      if (Math.ceil(maxValue / stepSize) + 1 <= maxLabelNum)
        break;
      // Check |stepSize| * 2.
      if (Math.ceil(maxValue / (stepSize * 2)) + 1 <= maxLabelNum) {
        stepSize *= 2;
        break;
      }
      // Check |stepSize| * 5.
      if (Math.ceil(maxValue / (stepSize * 5)) + 1 <= maxLabelNum) {
        stepSize *= 5;
        break;
      }
      stepSize *= 10;
      if (stepSizePrecision > 0)
        --stepSizePrecision;
    }

    // Set the max so it's an exact multiple of the chosen step size.
    this.topLabelValue_ = Math.ceil(maxValue / stepSize) * stepSize;
    const topLabelValue = this.topLabelValue_;

    // Create labels.
    for (let value = topLabelValue; value >= 0; value -= stepSize) {
      const unitStr = this.units_[this.currentUnit_];
      const label = value.toFixed(stepSizePrecision) + ' ' + unitStr;
      this.labels_.push(label);
    }
    const realTopValue =
        topLabelValue * Math.pow(this.unitScale_, this.currentUnit_);
    this.scale_ = this.height_ / realTopValue;
    this.isCache_ = true;
  }

  getLabels() {
    this.updateLabels_();
    return this.labels_;
  }

  getScale() {
    this.updateLabels_();
    return this.scale_;
  }
}

/**
 * @const
 */
LineChart.Label = Label_;
})();