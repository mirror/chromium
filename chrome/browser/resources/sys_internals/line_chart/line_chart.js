// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('LineChart', function() {
  'use strict';

  class LineChart_ {
    constructor(rootDiv) {
      this.rootDiv_ = rootDiv;
      this.menu_ = new LineChart.Menu(this.onMenuUpdate_.bind(this));

      this.canvas_ = createElementWithClassName('canvas', 'line-chart-canvas');
      this.initCanvas_();
      this.canvasStyle_ = window.getComputedStyle(this.canvas_);

      this.scrollbar_ = new LineChart.Scrollbar(this.update.bind(this));

      this.startTime_ = Date.now();
      this.endTime_ = 1;
      this.scale_ = LineChart.DEFAULT_SCALE;

      this.subCharts_ = [];

      this.chartUpdateTimer_ = 0;

      this.isDragging_ = false;
      this.dragX_ = 0;
      this.isTouching_ = false;
      this.touchX_ = 0;
      this.touchZoomBase_ = 0;

      const menuDiv = this.menu_.getRootDiv();
      this.rootDiv_.appendChild(menuDiv);

      const chartOuterDiv =
          createElementWithClassName('div', 'line-chart-canvas-outer');
      chartOuterDiv.appendChild(this.canvas_);
      const scrollBarDiv = this.scrollbar_.getRootDiv();
      chartOuterDiv.appendChild(scrollBarDiv);
      this.rootDiv_.appendChild(chartOuterDiv);

      window.addEventListener('resize', this.onResize_.bind(this));

      this.resize_();  // init;
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
      this.canvas_.style.margin = LineChart.CHART_MARGIN + 'px ' +
          LineChart.CHART_MARGIN + 'px ' +
          '0 ' + LineChart.CHART_MARGIN + 'px ';
    }

    getChartWidth_() {
      const timeRange = this.endTime_ - this.startTime_;

      // TODO : comment
      const numOfPoint = Math.floor(timeRange / this.scale_);
      const sampleRate = LineChart.SAMPLE_RATE;
      return numOfPoint - (numOfPoint - 1) % sampleRate;
    }

    getChartVisibleWidth() {
      return this.rootDiv_.offsetWidth - LineChart.CHART_MARGIN * 2 -
          this.menu_.getWidth();
    }

    getChartVisibleHeight() {
      return this.rootDiv_.offsetHeight - LineChart.CHART_MARGIN -
          this.scrollbar_.getHeight();
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

    zoom(rate) {
      const oldScale = this.scale_;
      this.scale_ *= rate;
      if (this.scale_ < LineChart.MIN_SCALE) {
        this.scale_ = LineChart.MIN_SCALE;
      }

      if (this.scale_ == oldScale)
        return;

      if (this.scrollbar_.isScrolledToRightEdge()) {
        this.updateScrollBar_();
        this.scrollbar_.scrollToRightEdge();
        this.update();
        return;
      }

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

    onMouseWheel_(event) {
      event.preventDefault();
      const whellX = event.wheelDeltaX / LineChart.MOUSE_WHEEL_UNITS_PER_CLICK;
      const whellY = event.wheelDeltaY / LineChart.MOUSE_WHEEL_UNITS_PER_CLICK;
      this.scroll(LineChart.MOUSE_WHEEL_SCROLL_RATE * whellX);
      this.zoom(Math.pow(LineChart.MOUSE_WHEEL_ZOOM_RATE, -whellY));
    }

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
      this.scroll(LineChart.MOUSE_DRAG_RATE * dragDeltaX);
      this.dragX_ = event.clientX;
    }

    onMouseUp_(event) {
      this.isDragging_ = false;
    }

    touchDistance(touchA, touchB) {
      const diffX = touchA.clientX - touchB.clientX;
      const diffY = touchA.clientY - touchB.clientY;
      return Math.sqrt(diffX * diffX + diffY * diffY);
    }

    onTouchStart_(event) {
      event.preventDefault();
      this.isTouching_ = true;
      const touches = event.targetTouches;
      if (touches.length == 1) {
        this.touchX_ = touches[0].clientX;
      } else if (touches.length == 2) {
        this.touchZoomBase_ = this.touchDistance(touches[0], touches[1]);
      }
    }

    onTouchMove_(event) {
      event.preventDefault();
      if (!this.isTouching_)
        return;
      const touches = event.targetTouches;
      if (touches.length == 1) {
        const dragDeltaX = touches[0].clientX - this.touchX_;
        // TODO : new static var
        this.scroll(LineChart.MOUSE_DRAG_RATE * dragDeltaX);
        this.touchX_ = touches[0].clientX;
      } else if (touches.length == 2) {
        const newDistance = this.touchDistance(touches[0], touches[1]);
        // TODO: static var 60
        const zoomDelta = (this.touchZoomBase_ - newDistance) / 60;
        // TODO : new static var
        this.zoom(Math.pow(LineChart.MOUSE_WHEEL_ZOOM_RATE, zoomDelta));
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

    onResize_() {
      this.resize_();
      this.update();
    }

    onMenuUpdate_() {
      this.resize_();
      this.update();
    }

    resize_() {
      const width = this.getChartVisibleWidth();
      const height = this.getChartVisibleHeight();
      if (this.canvas_.width == width && this.canvas_.height == height)
        return;

      console.log('Resize : ' + width + ' ' + height);
      this.canvas_.width = width;
      this.canvas_.height = height;
      const scrollBarWidth = width + 2 * LineChart.CHART_MARGIN;
      this.scrollbar_.resize(scrollBarWidth);
      this.updateScrollBar_();
    }

    setSubChart(align, units, unitScale) {
      const oldSubChart = this.subCharts_[align];
      if (oldSubChart) {
        const dataSeries = oldSubChart.getDataSeries();
        for (let i = 0; i < dataSeries.length; ++i) {
          this.menu_.removeDataSeries(dataSeries[i]);
        }
      }

      const label = new LineChart.Label(units, unitScale);
      this.subCharts_[align] = new LineChart.SubChart(label, align);
      this.update();
    }

    addDataSeries(align, dataSeries) {
      this.subCharts_[align].addDataSeries(dataSeries);
      this.menu_.addDataSeries(dataSeries);
      this.update();
    }

    updateEndTimeToNow() {
      this.endTime_ = Date.now();
      this.updateScrollBar_();
      this.update();
    }

    renderTimeLabels(context, width, height, fontHeight, startTime) {
      const sampleText = (new Date(startTime)).toLocaleTimeString();
      const minSpacing = context.measureText(sampleText).width +
          LineChart.MIN_TIME_LABEL_HORIZONTAL_SPACING;

      const TIME_STEP_UNITS = LineChart.TIME_STEP_UNITS;

      let timeStep = null;
      for (let i = 0; i < TIME_STEP_UNITS.length; ++i) {
        if (TIME_STEP_UNITS[i] / this.scale_ >= minSpacing) {
          timeStep = TIME_STEP_UNITS[i];
          break;
        }
      }
      // If no such value, give up.
      if (!timeStep)
        return;

      const textYPosition =
          height + fontHeight + LineChart.MIN_LABEL_VERTICAL_SPACING;
      context.textBaseline = 'bottom';
      context.textAlign = 'center';
      context.fillStyle = LineChart.TEXT_COLOR;
      context.strokeStyle = LineChart.GRID_COLOR;

      // Find the time for the first label.  This time is a perfect multiple of
      // timeStep because of how UTC times work. ???
      // TODO: fix comment
      let time = Math.ceil(startTime / timeStep) * timeStep;
      context.beginPath();
      while (true) {
        const x = Math.round((time - startTime) / this.scale_);
        if (x >= width)
          break;
        const text = (new Date(time)).toLocaleTimeString();
        context.fillText(text, x, textYPosition);
        context.moveTo(x, 0);
        context.lineTo(x, height - 1);
        time += timeStep;
      }
      context.stroke();
    }

    render() {
      console.log('Render!!!');
      const context = this.canvas_.getContext('2d');
      const width = this.canvas_.width;
      const height = this.canvas_.height;

      // Init context
      context.font = this.canvasStyle_.getPropertyValue('font');
      context.lineWidth = 2;
      context.lineCap = 'round';
      context.lineJoin = 'round';

      // Clear the canvas.
      context.fillStyle = LineChart.BACKGROUND_COLOR;
      context.fillRect(0, 0, width, height);

      // Try to get font height in pixels.  Needed for layout.
      const fontHeightString = context.font.match(/([0-9]+)px/)[1];
      const fontHeight = parseInt(fontHeightString);

      let position = this.scrollbar_.getPosition();
      if (this.scrollbar_.getRange() == 0) {
        position =
            this.getChartWidth_() - this.canvas_.width;  // TODO : comment
      }
      const visibleStartTime = this.startTime_ + position * this.scale_;

      const graphHeight =
          height - fontHeight - LineChart.MIN_LABEL_VERTICAL_SPACING;
      this.renderTimeLabels(
          context, width, graphHeight, fontHeight, visibleStartTime);

      // Draw outline of the main graph area.
      context.strokeStyle = LineChart.GRID_COLOR;
      context.strokeRect(0, 0, width - 1, graphHeight - 1);

      const subCharts = this.subCharts_;
      const sampleRate = LineChart.SAMPLE_RATE;
      const offset = position % sampleRate;
      for (let i = 0; i < subCharts.length; ++i) {
        if (subCharts[i] == undefined)
          continue;
        subCharts[i].setLayout(
            width, graphHeight, fontHeight, visibleStartTime, this.scale_,
            offset);
        subCharts[i].updateMaxValue();
        subCharts[i].renderLines(context);
        subCharts[i].renderLabels(context);
      }
    }

    update() {
      // TODO: comment
      clearTimeout(this.chartUpdateTimer_);
      this.chartUpdateTimer_ = setTimeout(this.render.bind(this));
    }
  }

  return {LineChart: LineChart_};
});