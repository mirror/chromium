// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

class LineChart_ {
  constructor(rootDiv) {
    /** @const {Element} */
    this.rootDiv_ = rootDiv;

    /**
     * The start time of the time line chart. (Unix time)
     * @const {number}
     */
    this.startTime_ = Date.now();
    /**
     * The end time of the time line chart. (Unix time)
     * @type {number}
     */
    this.endTime_ = this.startTime_ + 1;

    /**
     * The scale of the line chart. Milliseconds per pixel.
     * @type {number}
     */
    this.scale_ = LineChart.DEFAULT_SCALE;

    /**
     * |subChart| is the chart that all data series in it shares the same unit
     * label. There are two |SubChart| in |LineChart|, one's label align left,
     * another's align right. See |LineChart.SubChart|.
     * @type {Array<LineChart.SubChart>}
     */
    this.subCharts_ = [null, null];

    /**
     * Use a timer to avoid updating the graph multiple times in a single
     * operation.
     * @type {number}
     */
    this.chartUpdateTimer_ = 0;

    /* Dragging events status and touching events status. */
    /** @type {boolean} */
    this.isDragging_ = false;

    /** @type {number} */
    this.dragX_ = 0;

    /** @type {boolean} */
    this.isTouching_ = false;

    /** @type {number} */
    this.touchX_ = 0;

    /** @type {number} */
    this.touchZoomBase_ = 0;

    /**
     * The menu to control the visibility of data series. See |LineChart.Menu|.
     * @const {LineChart.Menu}
     */
    this.menu_ = new LineChart.Menu(this.onMenuUpdate_.bind(this));
    const menuDiv = this.menu_.getRootDiv();
    this.rootDiv_.appendChild(menuDiv);

    /** @const {Element} */
    const chartOuterDiv =
        createElementWithClassName('div', 'line-chart-canvas-outer');
    this.rootDiv_.appendChild(chartOuterDiv);

    /** @const {Element} */
    this.canvas_ = createElementWithClassName('canvas', 'line-chart-canvas');
    this.initCanvas_();

    /** @const {CSSStyleDeclaration} */
    this.canvasStyle_ = window.getComputedStyle(this.canvas_);
    chartOuterDiv.appendChild(this.canvas_);

    /**
     * A dummy scrollbar to scroll the line chart and to show the current
     * visible position of the line chair.
     * @const {LineChart.Scrollbar}
     */
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
    const /** string */ pxString = `${LineChart.CHART_MARGIN}px`;
    const /** string */ marginString = `${pxString} ${pxString} 0 ${pxString}`;
    this.canvas_.style.margin = marginString;
  }

  /**
   * Mouse and touchpad scroll event. Horizontal scroll for chart scrolling,
   * vertical scroll for chart zooming.
   * @param {Event} event
   */
  onMouseWheel_(event) {
    event.preventDefault();
    const whellX = event.wheelDeltaX / LineChart.MOUSE_WHEEL_UNITS;
    const whellY = event.wheelDeltaY / LineChart.MOUSE_WHEEL_UNITS;
    this.scroll(LineChart.MOUSE_WHEEL_SCROLL_RATE * whellX);
    this.zoom(Math.pow(LineChart.ZOOM_RATE, -whellY));
  }

  /* The following three functions handle mouse dragging event. */
  onMouseDown_(/** Event */ event) {
    event.preventDefault();
    this.isDragging_ = true;
    this.dragX_ = event.clientX;
  }

  onMouseMove_(/** Event */ event) {
    event.preventDefault();
    if (!this.isDragging_)
      return;
    const /** number */ dragDeltaX = event.clientX - this.dragX_;
    this.scroll(LineChart.DRAG_RATE * dragDeltaX);
    this.dragX_ = event.clientX;
  }

  onMouseUp_(/** Event */ event) {
    this.isDragging_ = false;
  }

  /**
   * Return the distance of two touch points.
   * @param {Touch} touchA
   * @param {Touch} touchB
   * @return {number}
   */
  touchDistance_(touchA, touchB) {
    const /** number */ diffX = touchA.clientX - touchB.clientX;
    const /** number */ diffY = touchA.clientY - touchB.clientY;
    return Math.sqrt(diffX * diffX + diffY * diffY);
  }

  /* The following four functions handle touch events. One finger for
   * scrolling, two finger for zooming.
   */
  onTouchStart_(/** Event*/ event) {
    event.preventDefault();
    this.isTouching_ = true;
    const /** TouchList */ touches = event.targetTouches;
    if (touches.length == 1) {
      this.touchX_ = touches[0].clientX;
    } else if (touches.length == 2) {
      this.touchZoomBase_ = this.touchDistance_(touches[0], touches[1]);
    }
  }

  onTouchMove_(/** Event*/ event) {
    event.preventDefault();
    if (!this.isTouching_)
      return;
    const /** TouchList */ touches = event.targetTouches;
    if (touches.length == 1) {
      const /** number */ dragDeltaX = this.touchX_ - touches[0].clientX;
      this.scroll(LineChart.DRAG_RATE * dragDeltaX);
      this.touchX_ = touches[0].clientX;
    } else if (touches.length == 2) {
      const /** number */ newDistance =
          this.touchDistance_(touches[0], touches[1]);
      const /** number */ zoomDelta =
          (this.touchZoomBase_ - newDistance) / LineChart.TOUCH_ZOOM_UNITS;
      this.zoom(Math.pow(LineChart.ZOOM_RATE, zoomDelta));
      this.touchZoomBase_ = newDistance;
    }
  }

  onTouchEnd_(/** Event*/ event) {
    event.preventDefault();
    this.isTouching_ = false;
  }

  onTouchCancel_(/** Event*/ event) {
    event.preventDefault();
    this.isTouching_ = false;
  }

  /**
   * Zoom the line chart by setting the |scale| to |rate| times.
   * @param {number} rate
   */
  zoom(rate) {
    const /** number */ oldScale = this.scale_;
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
    const /** number */ oldPosition = this.scrollbar_.getPosition();
    const /** number */ width = this.canvas_.width;
    const /** number */ visibleEndTime = oldScale * (oldPosition + width);
    const /** number */ newPosition =
        Math.round(visibleEndTime / this.scale_) - width;

    this.updateScrollBar_();
    this.scrollbar_.setPosition(newPosition);

    this.update();
  }

  /**
   * Scroll the line chart by moving forward |delta| pixels.
   * @param {number} delta
   */
  scroll(delta) {
    const /** number */ oldPosition = this.scrollbar_.getPosition();
    let /** number */ newPosition = oldPosition + Math.round(delta);

    this.scrollbar_.setPosition(newPosition);
    if (this.scrollbar_.getPosition() == oldPosition)
      return;

    this.update();
  }

  /**
   * Handle window resize event.
   */
  onResize_() {
    this.resize_();
    this.update();
  }

  /**
   * Handle |LineChart.Menu| update event.
   */
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
    const /** number */ scrollBarWidth = width + 2 * LineChart.CHART_MARGIN;
    this.scrollbar_.resize(scrollBarWidth);
    this.updateScrollBar_();
  }

  /**
   * Set the end time of the line chart to current time.
   */
  updateEndTimeToNow() {
    this.endTime_ = Date.now();
    this.updateScrollBar_();
    this.update();
  }

  /**
   * Update the scrollbar to the current line chart status after zooming,
   * scrolling... etc.
   */
  updateScrollBar_() {
    let /** number */ scrollRange = this.getChartWidth_() - this.canvas_.width;
    if (scrollRange < 0) {
      scrollRange = 0;
    }
    const /** boolean */ isScrolledToRightEdge =
        this.scrollbar_.isScrolledToRightEdge();
    this.scrollbar_.setRange(scrollRange);
    if (isScrolledToRightEdge && !this.isDragging_) {
      this.scrollbar_.scrollToRightEdge();
    }
  }

  /**
   * Get the whole line chart width, in pixel.
   * @return {number}
   */
  getChartWidth_() {
    const /** number */ timeRange = this.endTime_ - this.startTime_;

    const /** number */ numOfPixels = Math.floor(timeRange / this.scale_);
    const /** number */ sampleRate = LineChart.SAMPLE_RATE;
    /* To reduce CPU usage, the chart do not draw points at every pixels.
     * Remove the last few pixels to avoid the graph showing some blank at
     * the end of the graph. */
    const /** number */ extraPixels = (numOfPixels - 1) % sampleRate;
    return numOfPixels - extraPixels;
  }

  /**
   * Get the visible chart width, the width we need to render to the canvas, in
   * pixel.
   * @return {number}
   */
  getChartVisibleWidth() {
    return this.rootDiv_.offsetWidth - LineChart.CHART_MARGIN * 2 -
        this.menu_.getWidth();
  }

  /**
   * Get the visible chart height.
   * @return {number}
   */
  getChartVisibleHeight() {
    return this.rootDiv_.offsetHeight - LineChart.CHART_MARGIN -
        this.scrollbar_.getHeight();
  }

  /**
   * Set or reset the |units| and the |unitScale| of the |SubChart|.
   * @param {number} align - The align side of the subchart.
   * @param {Array<string>} units - See |LineChart.Label|.
   * @param {number} unitScale - See |LineChart.Label|.
   */
  setSubChart(align, units, unitScale) {
    this.clearSubChart(align);
    const /** LineChart.Label */ label = new LineChart.Label(units, unitScale);
    this.subCharts_[align] = new LineChart.SubChart(label, align);
    this.update();
  }

  /**
   * Clear all subcharts and data series in the line chart.
   */
  clearAllSubChart() {
    for (let /** number */ i = 0; i < this.subCharts_.length; ++i) {
      this.clearSubChart(i);
    }
    this.update();
  }

  /**
   * Clear a single subchart and its data series.
   * @param {number} align - The align side of the subchart.
   */
  clearSubChart(align) {
    const /** LineChart.SubChart */ oldSubChart = this.subCharts_[align];
    if (oldSubChart) {
      const /** Array<LineChart.DataSeries> */ dataSeries =
          oldSubChart.getDataSeries();
      for (let /** number */ i = 0; i < dataSeries.length; ++i) {
        this.menu_.removeDataSeries(dataSeries[i]);
      }
    }
    this.subCharts_[align] = null;
    this.update();
  }

  /**
   * Add a data series to a subchart of the line chart. Call |setSubChart|
   * before calling this function.
   * @param {number} align - The align side of the subchart.
   * @param {LineChart.DataSeries} dataSeries
   */
  addDataSeries(align, dataSeries) {
    const /** Array<LineChart.SubChart> */ subCharts = this.subCharts_;
    if (subCharts[align] == null) {
      console.warn(
          'This sub chart has not been setup yet. ' +
          'Call |setSubChart| before calling this function.');
      return;
    }
    this.subCharts_[align].addDataSeries(dataSeries);
    this.menu_.addDataSeries(dataSeries);
    this.update();
  }

  /**
   * Render the line chart. Note that to avoid calling render function
   * multiple times in a single operation, this function will set a timeout
   * rather than calling render function directly.
   */
  update() {
    clearTimeout(this.chartUpdateTimer_);
    if (!this.shouldBeRender())
      return;
    this.chartUpdateTimer_ = setTimeout(this.render_.bind(this));
  }

  /**
   * Return true if any subchart need to be rendered.
   * @return {boolean}
   */
  shouldBeRender() {
    const subCharts = this.subCharts_;
    for (let /** number */ i = 0; i < subCharts.length; ++i) {
      if (subCharts[i] != null && subCharts[i].shouldBeRender())
        return true;
    }
    return false;
  }

  /**
   * Implementation of line chart rendering.
   */
  render_() {
    const /** CanvasRenderingContext2D */ context =
        this.canvas_.getContext('2d');
    const /** number */ width = this.canvas_.width;
    const /** number */ height = this.canvas_.height;

    this.initAndClearContext_(context, width, height);

    const /** number */ fontHeight =
        this.getFontHeightFromStyleString_(context.font);
    if (fontHeight == 0) {
      console.warn(
          'Render failed. Cannot get the font height from the canvas ' +
          'font style string.');
      return;
    }

    let /** number */ position = this.scrollbar_.getPosition();
    if (this.scrollbar_.getRange() == 0) {
      /* If the chart width less than the visible width, make the chart align
       * right by setting the negative position. */
      position = this.getChartWidth_() - this.canvas_.width;
    }
    const /** number */ visibleStartTime =
        this.startTime_ + position * this.scale_;
    const /** number */ graphHeight =
        height - fontHeight - LineChart.MIN_LABEL_VERTICAL_SPACING;
    this.renderTimeLabels_(
        context, width, graphHeight, fontHeight, visibleStartTime);
    this.renderContour_(context, width, graphHeight);
    this.renderSubCharts_(
        context, width, graphHeight, fontHeight, visibleStartTime, position);
  }

  /**
   * @param {CanvasRenderingContext2D} context
   * @param {number} width
   * @param {number} height
   */
  initAndClearContext_(context, width, height) {
    context.font = this.canvasStyle_.getPropertyValue('font');
    context.lineWidth = 2;
    context.lineCap = 'round';
    context.lineJoin = 'round';
    context.fillStyle = LineChart.BACKGROUND_COLOR;
    context.fillRect(0, 0, width, height);
  }

  /**
   * Get the font height by parsing the font style string.
   * @param {string} fontStyleString
   * @return {number}
   */
  getFontHeightFromStyleString_(fontStyleString) {
    /* CSSStyleDeclaration will transform all font size unit to pixel. */
    const /** Array<string>|null */ match = fontStyleString.match(/([0-9]+)px/);
    if (!match)
      return 0;
    const /** number */ fontHeight = parseInt(match[1], 10);
    if (isNaN(fontHeight))
      return 0;
    return fontHeight;
  }

  /**
   * Render the time label under the line chart.
   * @param {CanvasRenderingContext2D} context
   * @param {number} width
   * @param {number} height
   * @param {number} fontHeight
   * @param {number} startTime - The start time of the time label.
   */
  renderTimeLabels_(context, width, height, fontHeight, startTime) {
    const /** string */ sampleText = (new Date(startTime)).toLocaleTimeString();
    const /** number */ minSpacing = context.measureText(sampleText).width +
        LineChart.MIN_TIME_LABEL_HORIZONTAL_SPACING;

    const /** Array<number> */ TIME_STEP_UNITS = LineChart.TIME_STEP_UNITS;

    let /** number */ timeStep = this.getSuitableTimeStep_(minSpacing);
    if (timeStep == 0) {
      console.warn('Render time label failed. Cannot find suitable time unit.');
      return;
    }

    context.textBaseline = 'bottom';
    context.textAlign = 'center';
    context.fillStyle = LineChart.TEXT_COLOR;
    context.strokeStyle = LineChart.GRID_COLOR;
    context.beginPath();
    const /** number */ yCoord =
        height + fontHeight + LineChart.MIN_LABEL_VERTICAL_SPACING;
    const /** number */ firstTimeTick =
        Math.ceil(startTime / timeStep) * timeStep;
    let /** number */ time = firstTimeTick;
    while (true) {
      const /** number */ xCoord = Math.round((time - startTime) / this.scale_);
      if (xCoord >= width)
        break;
      const /** string */ text = (new Date(time)).toLocaleTimeString();
      context.fillText(text, xCoord, yCoord);
      context.moveTo(xCoord, 0);
      context.lineTo(xCoord, height - 1);
      time += timeStep;
    }
    context.stroke();
  }

  /**
   * Find the suitable step of time to render the time label.
   * @param {number} minSpacing - The minimum spacing between two time tick.
   * @return {number}
   */
  getSuitableTimeStep_(minSpacing) {
    const /** Array<number> */ timeStepUnits = LineChart.TIME_STEP_UNITS;
    let /** number */ timeStep = 0;
    for (let /** number */ i = 0; i < timeStepUnits.length; ++i) {
      if (timeStepUnits[i] / this.scale_ >= minSpacing) {
        timeStep = timeStepUnits[i];
        break;
      }
    }
    return timeStep;
  }

  /**
   * @param {CanvasRenderingContext2D} context
   * @param {number} graphWidth
   * @param {number} graphHeight
   */
  renderContour_(context, graphWidth, graphHeight) {
    context.strokeStyle = LineChart.GRID_COLOR;
    context.strokeRect(0, 0, graphWidth - 1, graphHeight - 1);
  }

  /**
   * Render the subcharts and their data series.
   * @param {CanvasRenderingContext2D} context
   * @param {number} graphWidth
   * @param {number} graphHeight
   * @param {number} fontHeight
   * @param {number} visibleStartTime
   * @param {number} position - The scrollbar position.
   */
  renderSubCharts_(
      context, graphWidth, graphHeight, fontHeight, visibleStartTime,
      position) {
    const /** Array<LineChart.SubChart> */ subCharts = this.subCharts_;
    const /** number */ sampleRate = LineChart.SAMPLE_RATE;
    /* To reduce CPU usage, the chart do not draw points at every pixels. Use
     * |offset| to make sure the graph won't shaking during scrolling, the line
     * chart will render the data points at the same absolute position. */
    const /** number */ offset = position % sampleRate;
    for (let /** number */ i = 0; i < subCharts.length; ++i) {
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

/**
 * @const
 */
LineChart.LineChart = LineChart_;
})();
