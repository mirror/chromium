// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Single data type.(Single labels)
 */

cr.define('LineChart', function() {
  'use strict';

  class SubChart_ {
    constructor(label, labelAlign) {
      this.label_ = label;
      this.labelAlign_ = labelAlign;
      this.dataSeries_ = [];

      this.startTime_ = 0;
      this.scale_ = 1;
      this.width_ = 1;
      this.height_ = 1;

      this.maxValue = 0;
    }

    setLayout(width, height, fontHeight, startTime, scale) {
      // console.log('Layout' + width + ' '+ height);
      this.width_ = width;
      this.height_ = height;
      this.startTime_ = startTime;
      this.scale_ = scale;
      this.label_.setLayout(height, fontHeight, 2);
    }

    addDataSeries(dataSeries) {
      this.dataSeries_.push(dataSeries);
    }

    getDataSeries(dataSeries) {
      return this.dataSeries_;
    }

    getValuesFromDataSeries(dataSeries) {
      if (!dataSeries.isVisible())
        return null;
      return dataSeries.getValues(this.startTime_, this.scale_, this.width_);
    }

    getMaxValueFromDataSeries(dataSeries) {
      if (!dataSeries.isVisible())
        return 0;
      return dataSeries.getMaxValue(this.startTime_, this.scale_, this.width_);
    }

    updateMaxValue() {
      const dataSeries = this.dataSeries_;
      let maxValue = 0;
      for (let i = 0; i < dataSeries.length; ++i) {
        const value = this.getMaxValueFromDataSeries(dataSeries[i]);
        maxValue = Math.max(maxValue, value);
      }
      if (this.maxValue_ != maxValue) {
        this.label_.setMaxValue(maxValue);
        this.maxValue_ = maxValue;
      }
    }

    /**
     * Render all data series and label.
     */
    renderLines(context) {
      // console.log('render line');
      const dataSeries = this.dataSeries_;
      const valueScale = this.label_.getScale();

      // console.log('dataSeries len ' + dataSeries.length);
      for (let i = 0; i < dataSeries.length; ++i) {
        const values = this.getValuesFromDataSeries(dataSeries[i]);
        if (!values)
          continue;

        // console.log('values len' + values.length);
        context.strokeStyle = dataSeries[i].getColor();
        context.fillStyle = dataSeries[i].getColor();
        context.beginPath();
        let firstX = -1;
        for (let x = 0; x < values.length; ++x) {
          if (values[x] != null) {
            const yCoord = Math.round(values[x] * valueScale);
            const realYCoord = this.height_ - yCoord;
            context.lineTo(x, realYCoord);
            if (firstX == -1) {
              firstX = x;
            }
          }
        }
        context.stroke();
        context.lineTo(this.width_, this.height_);
        context.lineTo(firstX, this.height_);
        context.globalAlpha = 0.2;
        context.fill();
        context.globalAlpha = 1.0;
      }
    }

    renderLabels(context) {
      const labelTexts = this.label_.getLabels();
      if (labelTexts.length == 0)
        return;

      let x;
      let tickStartX;
      let tickEndX;
      if (this.labelAlign_ == LineChart.LabelAlign.LEFT) {
        context.textAlign = 'left';
        x = LineChart.MIN_LABEL_HORIZONTAL_SPACING;
        tickStartX = 0;
        tickEndX = LineChart.Y_AXIS_TICK_LENGTH;
      } else if (this.labelAlign_ == LineChart.LabelAlign.RIGHT) {
        context.textAlign = 'right';
        x = this.width_ - LineChart.MIN_LABEL_HORIZONTAL_SPACING;
        tickStartX = this.width_ - 1;
        tickEndX = this.width_ - 1 - LineChart.Y_AXIS_TICK_LENGTH;
      } else {
        console.warn('Unknown label align.');
        return;
      }

      context.fillStyle = LineChart.TEXT_COLOR;
      context.strokeStyle = LineChart.GRID_COLOR;

      context.textBaseline = 'top';
      context.fillText(labelTexts[0], x, 0);

      // Draw all the other labels.
      context.textBaseline = 'bottom';
      const labelYStep = this.height_ / (labelTexts.length - 1);
      context.beginPath();
      for (let i = 1; i < labelTexts.length; ++i) {
        const y = labelYStep * i;
        context.fillText(labelTexts[i], x, y);
        if (i < labelTexts.length - 1) {
          context.moveTo(tickStartX, y);
          context.lineTo(tickEndX, y);
        }
      }
      context.stroke();
    }
  }

  return {SubChart: SubChart_};
});