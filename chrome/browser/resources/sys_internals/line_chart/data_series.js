// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

class DataSeries_ {
  constructor(title, color) {
    /** @const {string} - The name of this data series. */
    this.title_ = title;

    /** @const {string} - The color of this data series. */
    this.color_ = color;

    /**
     * Whether the menu text color is black. To avoid showing white text on
     * light color.
     * @type {boolean}
     */
    this.isMenuTextBlack_ = false;

    /**
     * All the data points of this data series. Sorted by time.
     * @type {Array<{value: number, time: number}>}
     */
    this.dataPoints_ = [];

    /** @type {boolean} - To show or to hide this data series on the chart. */
    this.isVisible_ = true;

    /** @type {number} */
    this.cacheStartTime_ = 0;

    /** @type {number} */
    this.cacheStepSize_ = 0;

    /** @type {Array<number>} */
    this.cacheValues_ = [];

    /** @type {number} */
    this.cacheOffset_ = 0;

    /** @type {number} */
    this.cacheMaxValue_ = 0;
  }

  /**
   * Add a new data point to this data series. The time must greater than the
   * time of the last data point in the data series.
   * @param {number} value
   * @param {number} time
   */
  addDataPoint(value, time) {
    if (!isFinite(value) || !isFinite(time)) {
      console.warn('Add invalid value to DataSeries.');
      return;
    }
    const /** number */ length = this.dataPoints_.length;
    if (length > 0 && this.dataPoints_[length - 1].time > time) {
      console.warn(
          'Add invalid time to DataSeries: ' + time +
          '. Time must be non-strictly increasing.');
      return;
    }
    const /** {value: number, time: number} */ point = {
      value: value,
      time: time,
    };
    this.dataPoints_.push(point);
    this.clearCacheValue_();
  }

  /** See |updateCacheValues_()|*/
  clearCacheValue_() {
    this.cacheValues_ = [];
  }

  setMenuTextBlack(/** boolean */ isTextBlack) {
    this.isMenuTextBlack_ = isTextBlack;
  }

  setVisible(/** boolean */ isVisible) {
    this.isVisible_ = isVisible;
  }

  /** @return {boolean} */
  isVisible() {
    return this.isVisible_;
  }

  /** @return {boolean} */
  isMenuTextBlack() {
    return this.isMenuTextBlack_;
  }

  /** @return {string} */
  getTitle() {
    return this.title_;
  }

  /** @return {string} */
  getColor() {
    return this.color_;
  }

  /**
   * Get the values to draw on screen.
   * @param {number} startTime - The time of first point.
   * @param {number} stepSize - The step size between two value points.
   * @param {number} count - The number of values.
   * @return {Array<number|null>} - If a cell of the array is null, it means
   *      that there is no any data point in this interval.
   *      See |getSampleValue_()|.
   */
  getValues(startTime, stepSize, count) {
    this.updateCacheValues_(startTime, stepSize, count);
    return this.cacheValues_;
  }

  /**
   * Get the maximum value of all values in the query interval. See
   * |getValues()|.
   * @param {number} startTime
   * @param {number} stepSize
   * @param {number} count
   * @return {number}
   */
  getMaxValue(startTime, stepSize, count) {
    this.updateCacheValues_(startTime, stepSize, count);
    return this.cacheMaxValue_;
  }

  /**
   * Implementation of querying the values and the max value. See |getValues()|.
   * @param {number} startTime
   * @param {number} stepSize
   * @param {number} count
   */
  updateCacheValues_(startTime, stepSize, count) {
    if (this.cacheStartTime_ == startTime && this.cacheStepSize_ == stepSize &&
        this.cacheValues_.length == count)
      return;

    const /** Array<null|number> */ values = [];
    values.length = count;
    const /** number */ sampleRate = LineChart.SAMPLE_RATE;
    let /** number */ endTime = startTime;
    const /** number */ firstIndex = this.findFirstPointIndex_(startTime);
    let /** number */ nextIndex = firstIndex;
    let /** number */ maxValue = 0;

    for (let i = 0; i < count; ++i) {
      endTime += stepSize;
      const /** {value: (number|null), nextIndex: number} */ result =
          this.getSampleValue_(nextIndex, endTime);
      values[i] = result.value;
      nextIndex = result.nextIndex;
      maxValue = Math.max(maxValue, values[i]);
    }

    this.cacheValues_ = values;
    this.cacheStartTime_ = startTime;
    this.cacheStepSize_ = stepSize;
    this.cacheMaxValue_ = maxValue;
    this.fillUpFirstAndLastValuePoint_(firstIndex, nextIndex);
  }

  /**
   * Find the index of first point by simple binary search.
   * @param {number} time
   * @return {number}
   */
  findFirstPointIndex_(time) {
    let /** number */ lower = -1;
    let /** number */ upper = this.dataPoints_.length;
    while (lower < upper - 1) {
      const /** number */ mid = Math.floor((lower + upper) / 2);
      if (this.dataPoints_[mid].time < time) {
        lower = mid;
      } else {
        upper = mid;
      }
    }
    return upper;
  }

  /**
   * Get a single sample value from |firstIndex| to |endTime|. Return the value
   * and the next index of the points.
   * If there are many data points in the query interval, return their average.
   * If there is no any data point in the query interval, return null.
   * @param {number} firstIndex
   * @param {number} endTime
   * @return {{value: (number|null), nextIndex: number}}
   */
  getSampleValue_(firstIndex, endTime) {
    const /** Array<{value: number, time: number}> */ dataPoints =
        this.dataPoints_;
    let /** number */ nextIndex = firstIndex;
    let /** number */ currentValueSum = 0;
    let /** number */ currentPointNum = 0;
    while (nextIndex < dataPoints.length &&
           dataPoints[nextIndex].time < endTime) {
      currentValueSum += dataPoints[nextIndex].value;
      ++currentPointNum;
      ++nextIndex;
    }

    let /** number|null */ value = null;
    if (currentPointNum > 0) {
      value = currentValueSum / currentPointNum;
    }
    return {
      value: value,
      nextIndex: nextIndex,
    };
  }

  /**
   * Try to fill up the first point and the last point by the liner
   * interpolation if they are null. Therefore, the chart won't be broken beside
   * the edge of the chart.
   * @param {number} firstIndex
   * @param {number} lastIndex
   */
  fillUpFirstAndLastValuePoint_(firstIndex, lastIndex) {
    const /** Array<{value: number, time: number}> */ dataPoints =
        this.dataPoints_;
    const /** Array<number|null> */ values = this.cacheValues_;
    const /** number */ startTime = this.cacheStartTime_;
    let /** number */ maxValue = this.cacheMaxValue_;
    if (values[0] == null && firstIndex > 0) {
      values[0] = this.constructor.dataPointLinerInterpolation(
          dataPoints[firstIndex - 1], dataPoints[firstIndex], startTime);
      maxValue = Math.max(maxValue, values[0]);
    }
    const /** number */ count = this.cacheValues_.length;
    if (values[count - 1] == null && lastIndex > 0 &&
        lastIndex < dataPoints.length) {
      const /** number */ stepSize = this.cacheStepSize_;
      const /** number */ endTime = startTime + stepSize * count;
      values[count - 1] = this.constructor.dataPointLinerInterpolation(
          dataPoints[lastIndex - 1], dataPoints[lastIndex], endTime);
      maxValue = Math.max(maxValue, values[count - 1]);
    }
    this.cacheMaxValue_ = maxValue;
  }

  /**
   * Do the liner interpolation for the data points.
   * @param {{value: number, time: number}} pointA
   * @param {{value: number, time: number}} pointB
   * @param {number} position - The position we want to insert the new value.
   * @return {number}
   */
  static dataPointLinerInterpolation(pointA, pointB, position) {
    return this.linerInterpolation(
        pointA.time, pointA.value, pointB.time, pointB.value, position);
  }

  /**
   * The liner interpolation implementation.
   * @param {number} x1
   * @param {number} y1
   * @param {number} x2
   * @param {number} y2
   * @param {number} position
   * @return {number}
   */
  static linerInterpolation(x1, y1, x2, y2, position) {
    const /** number */ rate = (position - x1) / (x2 - x1);
    return (y2 - y1) * rate + y1;
  }
}

/**
 * @const
 */
LineChart.DataSeries = DataSeries_;
})();
