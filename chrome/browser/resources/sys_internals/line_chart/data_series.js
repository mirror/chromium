// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

class DataSeries_ {
  constructor(title, color) {
    this.title_ = title;
    this.color_ = color;
    this.isMenuTextBlack_ = false;
    this.dataPoints_ = [];
    this.isVisible_ = true;

    this.cacheStartTime_ = null;
    this.cacheStepSize_ = 0;
    this.cacheValues_ = [];
    this.cacheOffset_ = 0;
    this.cacheMaxValue_ = 0;
  }

  addDataPoint(value, time) {
    if (typeof(value) != 'number' || !isFinite(value) ||
        typeof(time) != 'number' || !isFinite(value)) {
      console.warn('Add invalid value to DataSeries: ' + String(value));
      return;
    }
    const length = this.dataPoints_.length;
    if (length > 0 && this.dataPoints_[length - 1].time > time) {
      console.warn(
          'Add invalid time to DataSeries: ' + time +
          '. Time must be non-strictly increasing.');
      return;
    }
    const point = {
      value: value,
      time: time,
    };
    this.dataPoints_.push(point);
    this.cacheValues_ = [];  // clear cache
  }

  isVisible() {
    return this.isVisible_;
  }

  setVisible(isVisible) {
    this.isVisible_ = isVisible;
  }

  setMenuTextBlack(isTextBlack) {
    this.isMenuTextBlack_ = isTextBlack;
  }

  getTitle() {
    return this.title_;
  }

  getColor() {
    return this.color_;
  }

  isMenuTextBlack() {
    return this.isMenuTextBlack_;
  }

  getValues(startTime, stepSize, count) {
    this.updateCacheValues(startTime, stepSize, count);
    return this.cacheValues_;
  }

  getMaxValue(startTime, stepSize, count) {
    this.updateCacheValues(startTime, stepSize, count);
    return this.cacheMaxValue_;
  }

  /**
   * TODO: JSDOC
   * Use average
   */
  updateCacheValues(startTime, stepSize, count) {
    if (this.cacheStartTime_ == startTime && this.cacheStepSize_ == stepSize &&
        this.cacheValues_.length == count)
      return;

    const values = [];
    const sampleRate = LineChart.SAMPLE_RATE;
    let time = startTime;
    const firstPoint = this.findFirstPoint(startTime);
    let nextPoint = firstPoint;
    let maxValue = 0;

    // TODO : comment
    for (let i = 0; i < count; ++i) {
      time += stepSize;
      [values[i], nextPoint] = this.getSampleValue_(nextPoint, time, stepSize);
      maxValue = Math.max(maxValue, values[i]);
    }

    const dataPoints = this.dataPoints_;
    if (values[0] == null && firstPoint > 0) {
      values[0] = this.constructor.dataPointLinerInterpolation(
          dataPoints[firstPoint - 1], dataPoints[firstPoint], startTime);
      maxValue = Math.max(maxValue, values[0]);
    }
    if (values[count - 1] == null && nextPoint > 0 &&
        nextPoint < dataPoints.length) {
      const endTime = startTime + stepSize * count;
      values[count - 1] = this.constructor.dataPointLinerInterpolation(
          dataPoints[nextPoint - 1], dataPoints[nextPoint], endTime);
      maxValue = Math.max(maxValue, values[count - 1]);
    }

    this.cacheValues_ = values;
    this.cacheStartTime_ = startTime;
    this.cacheStepSize_ = stepSize;
    this.cacheMaxValue_ = maxValue;
  }

  findFirstPoint(time) {
    let lower = -1;
    let upper = this.dataPoints_.length;
    while (lower < upper - 1) {
      const mid = Math.floor((lower + upper) / 2);
      if (this.dataPoints_[mid].time < time) {
        lower = mid;
      } else {
        upper = mid;
      }
    }
    return upper;
  }

  getSampleValue_(firstPoint, endTime, stepSize) {
    const dataPoints = this.dataPoints_;
    let nextPoint = firstPoint;
    let currentValueSum = 0;
    let currentPointNum = 0;
    while (nextPoint < dataPoints.length &&
           dataPoints[nextPoint].time < endTime) {
      currentValueSum += dataPoints[nextPoint].value;
      ++currentPointNum;
      ++nextPoint;
    }

    let value = null;
    if (currentPointNum > 0) {
      value = currentValueSum / currentPointNum;
    }
    return [value, nextPoint];
  }

  static dataPointLinerInterpolation(pointA, pointB, position) {
    return this.linerInterpolation(
        pointA.time, pointA.value, pointB.time, pointB.value, position);
  }

  /**
   * Comment
   */
  static linerInterpolation(x1, y1, x2, y2, position) {
    const rate = (position - x1) / (x2 - x1);
    return (y2 - y1) * rate + y1;
  }
}

/**
 * @const
 */
LineChart.DataSeries = DataSeries_;
})();