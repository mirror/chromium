// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('LineChart', function() {
  'use strict';

  class LineChart_ {
    constructor(rootDiv) {
      this.rootDiv_ = rootDiv;

      /* The start time and the end time of the whole chart. */
      this.startTime_ = Date.now();
      this.endTime_ = this.startTime_ + 1;

      /* Milliseconds per pixel. */
      this.scale_ = LineChart.DEFAULT_SCALE;

      /* Every subChart uses different units. */
      this.subCharts_ = [];

      /* Use a timer to avoid update the graph multiple times in a single
       * operation. */
      this.chartUpdateTimer_ = 0;

      this.isDragging_ = false;
      this.dragX_ = 0;
      this.isTouching_ = false;
      this.touchX_ = 0;
      this.touchZoomBase_ = 0;

      this.menu_ = new LineChart.Menu(this.onMenuUpdate_.bind(this));
      const menuDiv = this.menu_.getRootDiv();
      this.rootDiv_.appendChild(menuDiv);

      const chartOuterDiv =
          createElementWithClassName('div', 'line-chart-canvas-outer');
      this.rootDiv_.appendChild(chartOuterDiv);

      this.canvas_ = createElementWithClassName('canvas', 'line-chart-canvas');
      this.initCanvas_();
      this.canvasStyle_ = window.getComputedStyle(this.canvas_);
      chartOuterDiv.appendChild(this.canvas_);

      this.scrollbar_ = new LineChart.Scrollbar(this.update.bind(this));
      const scrollBarDiv = this.scrollbar_.getRootDiv();
      chartOuterDiv.appendChild(scrollBarDiv);

      window.addEventListener('resize', this.onResize_.bind(this));

      /* Initialize the graph size. */
      this.resize_();
    }

    initCanvas_() {
      this.canvas_.onmousewheel = this.onMouseWheel_.bind(this);
      this.canvas_.onmousedown = this.onMouseDown_.bind(this);
      this.canvas_.onmousemove = this.onMouseMove_.bind(this);
      this.canvas_.onmouseup = this.onMouseUp_.bind(this);
      this.canvas_.onmouseout = this.onMouseUp_.bind(this);
      this.canvas_.ontouchstart = this.onTouchStart_.bind(this);
      this.canvas_.ontouchmove = this.onTouchMove_.bind(this);
      this.canvas_.ontouchend = this.onTouchEnd_.bind(this);
      this.canvas_.ontouchcancel = this.onTouchCancel_.bind(this);
      const pxString = `${LineChart.CHART_MARGIN}px`;
      const marginString = `${pxString} ${pxString} 0 ${pxString}`;
      this.canvas_.style.margin = marginString;
    }

    /* Mouse and touchpad scroll event. Horizontal scroll for chart scrolling,
     * vertical scroll for chart zooming.
     */
    onMouseWheel_(event) {
      event.preventDefault();
      const whellX = event.wheelDeltaX / LineChart.MOUSE_WHEEL_UNITS;
      const whellY = event.wheelDeltaY / LineChart.MOUSE_WHEEL_UNITS;
      this.scroll(LineChart.MOUSE_WHEEL_SCROLL_RATE * whellX);
      this.zoom(Math.pow(LineChart.ZOOM_RATE, -whellY));
    }

    /* The following three functions handle mouse dragging event. */
    onMouseDown_(event) {
      event.preventDefault();
      this.isDragging_ = true;
      this.dragX_ = event.clientX;
    }

    onMouseMove_(event) {
      event.preventDefault();
      if (!this.isDragging_)
        return;
      const dragDeltaX = event.clientX - this.dragX_;
      this.scroll(LineChart.DRAG_RATE * dragDeltaX);
      this.dragX_ = event.clientX;
    }

    onMouseUp_(event) {
      this.isDragging_ = false;
    }

    touchDistance_(touchA, touchB) {
      const diffX = touchA.clientX - touchB.clientX;
      const diffY = touchA.clientY - touchB.clientY;
      return Math.sqrt(diffX * diffX + diffY * diffY);
    }

    /* The following four functions handle touch events. One finger for
     * scrolling, two finger for zooming.
     */
    onTouchStart_(event) {
      event.preventDefault();
      this.isTouching_ = true;
      const touches = event.targetTouches;
      if (touches.length == 1) {
        this.touchX_ = touches[0].clientX;
      } else if (touches.length == 2) {
        this.touchZoomBase_ = this.touchDistance_(touches[0], touches[1]);
      }
    }

    onTouchMove_(event) {
      event.preventDefault();
      if (!this.isTouching_)
        return;
      const touches = event.targetTouches;
      if (touches.length == 1) {
        const dragDeltaX = this.touchX_ - touches[0].clientX;
        this.scroll(LineChart.DRAG_RATE * dragDeltaX);
        this.touchX_ = touches[0].clientX;
      } else if (touches.length == 2) {
        const newDistance = this.touchDistance_(touches[0], touches[1]);
        const zoomDelta =
            (this.touchZoomBase_ - newDistance) / LineChart.TOUCH_ZOOM_UNITS;
        this.zoom(Math.pow(LineChart.ZOOM_RATE, zoomDelta));
        this.touchZoomBase_ = newDistance;
      }
    }

    onTouchEnd_(event) {
      event.preventDefault();
      this.isTouching_ = false;
    }

    onTouchCancel_(event) {
      event.preventDefault();
      this.isTouching_ = false;
    }

    zoom(rate) {
      const oldScale = this.scale_;
      this.scale_ *= rate;
      if (this.scale_ < LineChart.MIN_SCALE) {
        this.scale_ = LineChart.MIN_SCALE;
      } else if (this.scale_ > LineChart.MAX_SCALE) {
        this.scale_ = LineChart.MAX_SCALE;
      }

      if (this.scale_ == oldScale)
        return;

      if (this.scrollbar_.isScrolledToRightEdge()) {
        this.updateScrollBar_();
        this.scrollbar_.scrollToRightEdge();
        this.update();
        return;
      }

      /* To try to make the chart keep right, make the right edge of the chart
       * stop at the same position. */
      const oldPosition = this.scrollbar_.getPosition();
      const width = this.canvas_.width;
      const visibleEndTime = oldScale * (oldPosition + width);
      const newPosition = Math.round(visibleEndTime / this.scale_) - width;

      this.updateScrollBar_();
      this.scrollbar_.setPosition(newPosition);

      this.update();
    }

    scroll(delta) {
      const oldPosition = this.scrollbar_.getPosition();
      let newPosition = oldPosition + Math.round(delta);

      this.scrollbar_.setPosition(newPosition);
      if (this.scrollbar_.getPosition() == oldPosition)
        return;

      this.update();
    }

    /* Handle window resize event. */
    onResize_() {
      this.resize_();
      this.update();
    }

    /* Handle |LineChart.Menu| update event. */
    onMenuUpdate_() {
      this.resize_();
      this.update();
    }

    resize_() {
      const width = this.getChartVisibleWidth();
      const height = this.getChartVisibleHeight();
      if (this.canvas_.width == width && this.canvas_.height == height)
        return;

      this.canvas_.width = width;
      this.canvas_.height = height;
      const scrollBarWidth = width + 2 * LineChart.CHART_MARGIN;
      this.scrollbar_.resize(scrollBarWidth);
      this.updateScrollBar_();
    }

    updateEndTimeToNow() {
      this.endTime_ = Date.now();
      this.updateScrollBar_();
      this.update();
    }

    updateScrollBar_() {
      let scrollRange = this.getChartWidth_() - this.canvas_.width;
      if (scrollRange < 0) {
        scrollRange = 0;
      }
      const isScrolledToRightEdge = this.scrollbar_.isScrolledToRightEdge();
      this.scrollbar_.setRange(scrollRange);
      if (isScrolledToRightEdge && !this.isDragging_) {
        this.scrollbar_.scrollToRightEdge();
      }
    }

    getChartWidth_() {
      const timeRange = this.endTime_ - this.startTime_;

      const numOfPixels = Math.floor(timeRange / this.scale_);
      const sampleRate = LineChart.SAMPLE_RATE;
      /* To reduce CPU usage, the chart do not draw points at every pixels.
       * Remove the last few pixels to avoid the graph showing some blank at
       * the end of the graph. */
      const extraPixels = (numOfPixels - 1) % sampleRate;
      return numOfPixels - extraPixels;
    }

    getChartVisibleWidth() {
      return this.rootDiv_.offsetWidth - LineChart.CHART_MARGIN * 2 -
          this.menu_.getWidth();
    }

    getChartVisibleHeight() {
      return this.rootDiv_.offsetHeight - LineChart.CHART_MARGIN -
          this.scrollbar_.getHeight();
    }

    setSubChart(align, units, unitScale) {
      this.clearSubChart(align);
      const label = new LineChart.Label(units, unitScale);
      this.subCharts_[align] = new LineChart.SubChart(label, align);
      this.update();
    }

    clearAllSubChart() {
      for (let i = 0; i < this.subCharts_.length; ++i) {
        this.clearSubChart(i);
      }
      this.update();
    }

    clearSubChart(align) {
      const oldSubChart = this.subCharts_[align];
      if (oldSubChart) {
        const dataSeries = oldSubChart.getDataSeries();
        for (let i = 0; i < dataSeries.length; ++i) {
          this.menu_.removeDataSeries(dataSeries[i]);
        }
      }
      this.subCharts_[align] = undefined;
      this.update();
    }

    addDataSeries(align, dataSeries) {
      this.subCharts_[align].addDataSeries(dataSeries);
      this.menu_.addDataSeries(dataSeries);
      this.update();
    }

    update() {
      /* Set a timeout to call the render function to avoid the render function
       * being called multiple times in a single operation. */
      clearTimeout(this.chartUpdateTimer_);
      if (!this.shouldBeRender())
        return;
      this.chartUpdateTimer_ = setTimeout(this.render_.bind(this));
    }

    shouldBeRender() {
      const subCharts = this.subCharts_;
      for (let i = 0; i < subCharts.length; ++i) {
        if (subCharts[i] != undefined && subCharts[i].shouldBeRender())
          return true;
      }
      return false;
    }

    render_() {
      const context = this.canvas_.getContext('2d');
      const width = this.canvas_.width;
      const height = this.canvas_.height;

      this.initAndClearContext_(context, width, height);

      const fontHeight = this.getFontHeightFromStyleString_(context.font);
      if (!fontHeight) {
        console.warn(
            'Render failed. Cannot get the font height from the canvas ' +
            'font style string.');
        return;
      }

      let position = this.scrollbar_.getPosition();
      if (this.scrollbar_.getRange() == 0) {
        /* If the chart width less than the visible width, make the chart keep
         * right. */
        position = this.getChartWidth_() - this.canvas_.width;
      }
      const visibleStartTime = this.startTime_ + position * this.scale_;
      const graphHeight =
          height - fontHeight - LineChart.MIN_LABEL_VERTICAL_SPACING;
      this.renderTimeLabels_(
          context, width, graphHeight, fontHeight, visibleStartTime);
      this.renderContour_(context, width, graphHeight);
      this.renderSubCharts_(
          context, width, graphHeight, fontHeight, visibleStartTime, position);
    }

    initAndClearContext_(context, width, height) {
      context.font = this.canvasStyle_.getPropertyValue('font');
      context.lineWidth = 2;
      context.lineCap = 'round';
      context.lineJoin = 'round';
      context.fillStyle = LineChart.BACKGROUND_COLOR;
      context.fillRect(0, 0, width, height);
    }

    getFontHeightFromStyleString_(fontStyleString) {
      const match = fontStyleString.match(/([0-9]+)px/);
      if (!match)
        return undefined;
      const fontHeight = parseInt(match[1]);
      if (isNaN(fontHeight))
        return undefined;
      return fontHeight;
    }

    renderTimeLabels_(context, width, height, fontHeight, startTime) {
      const sampleText = (new Date(startTime)).toLocaleTimeString();
      const minSpacing = context.measureText(sampleText).width +
          LineChart.MIN_TIME_LABEL_HORIZONTAL_SPACING;

      const TIME_STEP_UNITS = LineChart.TIME_STEP_UNITS;

      let timeStep = this.getSuitableTimeStep_(minSpacing);
      if (!timeStep) {
        console.warn(
            'Render time label failed. Cannot find suitable time unit.');
        return;
      }

      context.textBaseline = 'bottom';
      context.textAlign = 'center';
      context.fillStyle = LineChart.TEXT_COLOR;
      context.strokeStyle = LineChart.GRID_COLOR;
      context.beginPath();
      const yCoord = height + fontHeight + LineChart.MIN_LABEL_VERTICAL_SPACING;
      const firstTimeTick = Math.ceil(startTime / timeStep) * timeStep;
      let time = firstTimeTick;
      while (true) {
        const xCoord = Math.round((time - startTime) / this.scale_);
        if (xCoord >= width)
          break;
        const text = (new Date(time)).toLocaleTimeString();
        context.fillText(text, xCoord, yCoord);
        context.moveTo(xCoord, 0);
        context.lineTo(xCoord, height - 1);
        time += timeStep;
      }
      context.stroke();
    }

    getSuitableTimeStep_(minSpacing) {
      const TIME_STEP_UNITS = LineChart.TIME_STEP_UNITS;
      let timeStep = null;
      for (let i = 0; i < TIME_STEP_UNITS.length; ++i) {
        if (TIME_STEP_UNITS[i] / this.scale_ >= minSpacing) {
          timeStep = TIME_STEP_UNITS[i];
          break;
        }
      }
      return timeStep;
    }

    renderContour_(context, graphWidth, graphHeight) {
      context.strokeStyle = LineChart.GRID_COLOR;
      context.strokeRect(0, 0, graphWidth - 1, graphHeight - 1);
    }

    renderSubCharts_(
        context, graphWidth, graphHeight, fontHeight, visibleStartTime,
        position) {
      const subCharts = this.subCharts_;
      const sampleRate = LineChart.SAMPLE_RATE;
      /* To reduce CPU usage, the chart do not draw points at every pixels. Use
       * |offset| to make sure the graph won't shaking during scrolling. */
      const offset = position % sampleRate;
      for (let i = 0; i < subCharts.length; ++i) {
        if (subCharts[i] == undefined)
          continue;
        subCharts[i].setLayout(
            graphWidth, graphHeight, fontHeight, visibleStartTime, this.scale_,
            offset);
        subCharts[i].updateMaxValue();
        subCharts[i].renderLines(context);
        subCharts[i].renderLabels(context);
      }
    }
  }

  return {LineChart: LineChart_};
});