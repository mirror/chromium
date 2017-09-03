// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Single data type.(Single labels)
 */

(function() {
'use strict';

class SubChart_ {
  constructor(label, labelAlign) {
    /** @const {LineChart.Label} */
    this.label_ = label;

    /** @const {number} */
    this.labelAlign_ = labelAlign;

    /** @type {Array<LineChart.DataSeries>} */
    this.dataSeriesList_ = [];

    /**
     * The time of the visible left edge of the chart.
     * @type {number}
     */
    this.startTime_ = 0;

    /**
     * The step size between two data points, in millisecond.
     * @type {number}
     */
    this.stepSize_ = 1;

    /**
     * The time to query the data. It will be smaller than the |startTime|, so
     * the line chart won't leave blanks beside the edge of the chart.
     * @type {number}
     */
    this.queryStartTime_ = 0;

    /**
     * The scale of the line chart. Milliseconds per pixel.
     * @type {number}
     */
    this.scale_ = 1;

    /**
     * The offset of the current visible range. To make sure we draw the data
     * points at the same absolute postition. See also
     * |LineChart.renderSubCharts_()|.
     * @type {number}
     */
    this.offset_ = 0;

    /** @type {number} */
    this.width_ = 1;

    /** @type {number} */
    this.height_ = 1;

    /**
     * Number of the points need to be draw on the screen.
     * @type {number}
     */
    this.numOfPoint_ = 0;

    /**
     * The max value of all data points in the visible range of the data series
     * of this sub chart.
     * @type {number}
     */
    this.maxValue_ = 0;
  }

  /**
   * Set the layout of this sub chart. Call this function when something changed
   * like the window size, visible range, scale ...etc.
   * @param {number} width
   * @param {number} height
   * @param {number} fontHeight
   * @param {number} startTime
   * @param {number} scale
   * @param {number} offset
   */
  setLayout(width, height, fontHeight, startTime, scale, offset) {
    this.width_ = width;
    this.height_ = height;
    this.startTime_ = startTime;
    this.scale_ = scale;
    this.offset_ = offset;
    const /** number */ sampleRate = LineChart.SAMPLE_RATE;

    /* Draw a data point on every |sampleRate| pixels. */
    this.stepSize_ = scale * sampleRate;

    /* First point's position(|queryStartTime|) may go out of the canvas, to
     * make the line chart continuous at the begin of the visible range, as well
     * as the last points. */
    this.queryStartTime_ = startTime - offset * scale;
    const /** number */ firstAndLastPoints = 2;
    this.numOfPoint_ = Math.floor(width / sampleRate) + firstAndLastPoints;

    this.label_.setLayout(height, fontHeight, /* precision */ 2);
    this.updateMaxValue_();
  }

  /**
   * Calculate the max value for the current layout.
   */
  updateMaxValue_() {
    const /** Array<LineChart.DataSeries> */ dataSeriesList =
        this.dataSeriesList_;
    let /** number */ maxValue = 0;
    for (let /** number */ i = 0; i < dataSeriesList.length; ++i) {
      const value = this.getMaxValueFromDataSeries_(dataSeriesList[i]);
      maxValue = Math.max(maxValue, value);
    }
    if (this.maxValue_ != maxValue) {
      this.label_.setMaxValue(maxValue);
      this.maxValue_ = maxValue;
    }
  }

  /**
   * Query the max value of the query range from the data series.
   * @param {LineChart.DataSeries} dataSeries
   * @return {number}
   */
  getMaxValueFromDataSeries_(dataSeries) {
    if (!dataSeries.isVisible())
      return 0;
    return dataSeries.getMaxValue(
        this.queryStartTime_, this.stepSize_, this.numOfPoint_);
  }

  /**
   * Add a data series to this sub chart.
   * @param {LineChart.DataSeries} dataSeries
   */
  addDataSeries(dataSeries) {
    this.dataSeriesList_.push(dataSeries);
  }

  /**
   * Get all data series of this sub chart.
   * @return {Array<LineChart.DataSeries>}
   */
  getDataSeriesList() {
    return this.dataSeriesList_;
  }

  /**
   * Render the lines of all data series.
   * @param {CanvasRenderingContext2D} context
   */
  renderLines(context) {
    const /** Array<LineChart.DataSeries> */ dataSeriesList =
        this.dataSeriesList_;
    for (let /** number */ i = 0; i < dataSeriesList.length; ++i) {
      const /** Array<number> */ values =
          this.getValuesFromDataSeries_(dataSeriesList[i]);
      if (!values)
        continue;
      this.renderLineOfDataSeries_(context, dataSeriesList[i], values);
    }
  }

  /**
   * Query the the data points' values from the data series.
   * @param {LineChart.DataSeries} dataSeries
   * @return {Array<number>}
   */
  getValuesFromDataSeries_(dataSeries) {
    if (!dataSeries.isVisible())
      return [];
    return dataSeries.getValues(
        this.queryStartTime_, this.stepSize_, this.numOfPoint_);
  }

  /**
   * @param {CanvasRenderingContext2D} context
   * @param {LineChart.DataSeries} dataSeries
   * @param {Array<number>} values
   */
  renderLineOfDataSeries_(context, dataSeries, values) {
    context.strokeStyle = dataSeries.getColor();
    context.fillStyle = dataSeries.getColor();
    context.beginPath();

    const /** number */ sampleRate = LineChart.SAMPLE_RATE;
    const /** number */ valueScale = this.label_.getScale();
    let /** number */ firstXCoord = this.width_;
    let /** number */ xCoord = -this.offset_;
    for (let /** number */ i = 0; i < values.length; ++i) {
      if (values[i] != null) {
        const /** number */ chartYCoord = Math.round(values[i] * valueScale);
        const /** number */ realYCoord = this.height_ - chartYCoord;
        context.lineTo(xCoord, realYCoord);
        if (firstXCoord > xCoord) {
          firstXCoord = xCoord;
        }
      }
      xCoord += sampleRate;
    }
    context.stroke();
    this.fillAreaBelowLine_(context, firstXCoord);
  }

  /**
   * @param {CanvasRenderingContext2D} context
   * @param {number} firstXCoord
   */
  fillAreaBelowLine_(context, firstXCoord) {
    context.lineTo(this.width_, this.height_);
    context.lineTo(firstXCoord, this.height_);
    context.globalAlpha = 0.2;
    context.fill();
    context.globalAlpha = 1.0;
  }

  /**
   * @param {CanvasRenderingContext2D} context
   */
  renderLabels(context) {
    const /** Array<string> */ labelTexts = this.label_.getLabels();
    if (labelTexts.length == 0)
      return;

    let /** number */ tickStartX;
    let /** number */ tickEndX;
    let /** number */ textXCoord;
    if (this.labelAlign_ == LineChart.LabelAlign.LEFT) {
      context.textAlign = 'left';
      tickStartX = 0;
      tickEndX = LineChart.Y_AXIS_TICK_LENGTH;
      textXCoord = LineChart.MIN_LABEL_HORIZONTAL_SPACING;
    } else if (this.labelAlign_ == LineChart.LabelAlign.RIGHT) {
      context.textAlign = 'right';
      tickStartX = this.width_ - 1;
      tickEndX = this.width_ - 1 - LineChart.Y_AXIS_TICK_LENGTH;
      textXCoord = this.width_ - LineChart.MIN_LABEL_HORIZONTAL_SPACING;
    } else {
      console.warn('Unknown label align.');
      return;
    }
    const /** number */ labelYStep = this.height_ / (labelTexts.length - 1);
    this.renderLabelTicks_(
        context, labelTexts, labelYStep, tickStartX, tickEndX);
    this.renderLabelTexts_(context, labelTexts, labelYStep, textXCoord);
  }

  /**
   * Render the tick line for the unit label.
   * @param {CanvasRenderingContext2D} context
   * @param {Array<string>} labelTexts
   * @param {number} labelYStep
   * @param {number} tickStartX
   * @param {number} tickEndX
   */
  renderLabelTicks_(context, labelTexts, labelYStep, tickStartX, tickEndX) {
    context.strokeStyle = LineChart.GRID_COLOR;
    context.beginPath();
    /* First and last tick are the top and the bottom of the line chart, so
     * don't draw them again. */
    for (let /** number */ i = 1; i < labelTexts.length - 1; ++i) {
      const /** number */ yCoord = labelYStep * i;
      context.moveTo(tickStartX, yCoord);
      context.lineTo(tickEndX, yCoord);
    }
    context.stroke();
  }

  /**
   * Render the texts for the unit label.
   * @param {CanvasRenderingContext2D} context
   * @param {Array<string>} labelTexts
   * @param {number} labelYStep
   * @param {number} textXCoord
   */
  renderLabelTexts_(context, labelTexts, labelYStep, textXCoord) {
    /* The first label cannot align the bottom of the tick or it will go outside
     * the canvas. */
    context.fillStyle = LineChart.TEXT_COLOR;
    context.textBaseline = 'top';
    context.fillText(labelTexts[0], textXCoord, 0);

    context.textBaseline = 'bottom';
    for (let /** number */ i = 1; i < labelTexts.length; ++i) {
      const /** number */ yCoord = labelYStep * i;
      context.fillText(labelTexts[i], textXCoord, yCoord);
    }
  }

  /**
   * Return true if there is any data series in this sub chart, whaterver there
   * are visible or not.
   * @return {boolean}
   */
  shouldBeRender() {
    return this.dataSeriesList_.length > 0;
  }
}

/**
 * @const
 */
LineChart.SubChart = SubChart_;
})();
