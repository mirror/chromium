// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @unrestricted
 */
Timeline.PerformanceMonitor = class extends UI.VBox {
  constructor() {
    super(true);
    this.registerRequiredCSS('timeline/performanceMonitor.css');
    this._model = SDK.targetManager.mainTarget().model(SDK.PerformanceModel);
    this._canvas = /** @type {!HTMLCanvasElement} */ (this.contentElement.createChild('canvas'));
    /** @type {!Array<!{timestamp: number, metrics: !Map<string, number>}>} */
    this._metricsBuffer = [];
    /** @const */
    this._pixelsPerMs = 20 / 1e3;
    /** @const */
    this._pollIntervalMs = 500;
    /** @type {!Array<string>} */
    this._enabledMetrics = ['NodeCount', 'ScriptDuration', 'TaskDuration'];
    /** @type {!Map<string, !Timeline.PerformanceMonitor.Info>} */
    this._metricsInfo = new Map([
      [
        'TaskDuration', {
          title: Common.UIString('CPU Utilization'),
          color: 'red',
          mode: Timeline.PerformanceMonitor.Mode.CumulativeTime,
          min: 0,
          max: 100,
          format: formatPercent
        }
      ],
      [
        'ScriptDuration', {
          title: Common.UIString('Script Execution'),
          color: 'orange',
          mode: Timeline.PerformanceMonitor.Mode.CumulativeTime,
          min: 0,
          max: 100,
          format: formatPercent
        }
      ],
      ['NodeCount', {title: Common.UIString('DOM Nodes'), color: 'green'}],
      ['JSEventListenerCount', {title: Common.UIString('JS Event Listeners'), color: 'deeppink'}],
    ]);

    /**
     * @param {number} value
     * @return {string}
     */
    function formatPercent(value) {
      return value.toFixed(0) + '%';
    }
  }

  /**
   * @override
   */
  wasShown() {
    this._model.enable();
    this._pollTimer = setInterval(() => this._poll(), this._pollIntervalMs);
    this.onResize();
    animate.call(this);

    /**
     * @this {Timeline.PerformanceMonitor}
     */
    function animate() {
      this._update();
      this._animationId = this.contentElement.window().requestAnimationFrame(animate.bind(this));
    }
  }

  /**
   * @override
   */
  willHide() {
    clearInterval(this._pollTimer);
    this.contentElement.window().cancelAnimationFrame(this._animationId);
    this._model.disable();
    this._metricsBuffer = [];
  }

  async _poll() {
    var metrics = await this._model.requestMetrics();
    this._processMetrics(metrics);
    this._update();
  }

  /**
   * @param {!Array<!Protocol.Performance.Metric>} metrics
   */
  _processMetrics(metrics) {
    var metricsMap = new Map();
    var timestamp = Date.now();
    for (var metric of metrics) {
      var info = this._getMetricInfo(metric.name);
      var value;
      if (info.mode === Timeline.PerformanceMonitor.Mode.CumulativeTime) {
        value = info.lastTimestamp ?
            100 * Math.min(1, (metric.value - info.lastValue) / (timestamp - info.lastTimestamp) * 1e3) :
            0;
        info.lastValue = metric.value;
        info.lastTimestamp = timestamp;
      } else {
        value = metric.value;
      }
      metricsMap.set(metric.name, value);
    }
    this._metricsBuffer.push({timestamp, metrics: metricsMap});
    var millisPerWidth = this._width / this._pixelsPerMs;
    // Multiply by 2 as the pollInterval has some jitter.
    var maxCount = Math.ceil(millisPerWidth / this._pollIntervalMs * 2);
    if (this._metricsBuffer.length > maxCount * 2)
      this._metricsBuffer.splice(0, this._metricsBuffer.length - maxCount);
  }

  /**
   * @param {string} name
   * @return {!Timeline.PerformanceMonitor.Info}
   */
  _getMetricInfo(name) {
    if (!this._metricsInfo.has(name))
      this._metricsInfo.set(name, {title: name, color: 'grey'});
    return this._metricsInfo.get(name);
  }

  _update() {
    this._draw();
  }

  _draw() {
    var ctx = /** @type {!CanvasRenderingContext2D} */ (this._canvas.getContext('2d'));
    ctx.save();
    ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
    ctx.clearRect(0, 0, this._width, this._height);
    this._drawGrid(ctx);
    for (var i = 0; i < this._enabledMetrics.length; ++i)
      this._drawMetric(ctx, this._enabledMetrics[i]);
    this._drawLegend(ctx);
    ctx.restore();
  }

  /**
   * @param {!CanvasRenderingContext2D} ctx
   */
  _drawGrid(ctx) {
    ctx.font = '10px ' + Host.fontFamily();
    ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
    for (var sec = 0; sec < 500; ++sec) {
      var x = this._width - sec * this._pixelsPerMs * 1e3;
      if (x < 0)
        break;
      ctx.beginPath();
      ctx.moveTo(Math.round(x) + 0.5, 0);
      ctx.lineTo(Math.round(x) + 0.5, this._height);
      if (sec % 5 === 0)
        ctx.fillText(Common.UIString('%d sec', sec), Math.round(x) + 4, 12);
      ctx.strokeStyle = `hsla(0, 0%, 0%, ${sec % 5 ? 0.02 : 0.08})`;
      ctx.stroke();
    }
  }

  /**
   * @param {!CanvasRenderingContext2D} ctx
   * @param {string} metricName
   */
  _drawMetric(ctx, metricName) {
    ctx.save();
    var width = this._width;
    var height = this._height;
    var startTime = Date.now() - this._pollIntervalMs * 2 - width / this._pixelsPerMs;
    var info = this._getMetricInfo(metricName);
    var min = Infinity;
    var max = -Infinity;
    var pixelsPerMs = this._pixelsPerMs;
    if (info.min || info.max) {
      min = info.min;
      max = info.max;
    } else {
      for (var i = this._metricsBuffer.length - 1; i >= 0; --i) {
        var metrics = this._metricsBuffer[i];
        var value = metrics.metrics.get(metricName);
        min = Math.min(min, value);
        max = Math.max(max, value);
        if (metrics.timestamp < startTime)
          break;
      }
      if (isFinite(min) && isFinite(max)) {
        var alpha = 0.1;
        info.currentMin = min * alpha + (info.currentMin || min) * (1 - alpha);
        info.currentMax = max * alpha + (info.currentMax || max) * (1 - alpha);
        min = info.currentMin;
        max = info.currentMax;
      }
    }
    var span = 1.15 * (max - min) || 1;
    ctx.beginPath();
    ctx.moveTo(width + 5, height + 5);
    var x = 0;
    var lastY = 0;
    var lastX = 0;
    if (this._metricsBuffer.length) {
      lastY = calcY(this._metricsBuffer.peekLast().metrics.get(metricName));
      lastX = width + 5;
      ctx.lineTo(lastX, lastY);
    }
    for (var i = this._metricsBuffer.length - 1; i >= 0; --i) {
      var metrics = this._metricsBuffer[i];
      var y = calcY(metrics.metrics.get(metricName));
      x = (metrics.timestamp - startTime) * pixelsPerMs;
      var midX = (lastX + x) / 2;
      ctx.bezierCurveTo(midX, lastY, midX, y, x, y);
      lastX = x;
      lastY = y;
      if (metrics.timestamp < startTime)
        break;
    }
    ctx.lineTo(x, height + 5);
    var color = this._getMetricInfo(metricName).color || 'grey';
    ctx.strokeStyle = color;
    ctx.lineWidth = 0.5;
    ctx.stroke();
    ctx.globalAlpha = 0.05;
    ctx.fillStyle = color;
    ctx.fill();
    ctx.restore();

    /**
     * @param {number} value
     * @return {number}
     */
    function calcY(value) {
      return Math.round(height - 5 - height * (value - min) / span) + 0.5;
    }
  }

  /**
   * @param {!CanvasRenderingContext2D} ctx
   */
  _drawLegend(ctx) {
    ctx.font = '12px ' + Host.fontFamily();
    var topPadding = 20;
    var leftPadding = 10;
    var intervalPx = 18;
    var textColor = '#333';
    var numberFormat = new Intl.NumberFormat('en-US');
    ctx.fillStyle = 'hsla(0, 0%, 100%, 0.9)';
    ctx.fillRect(leftPadding, topPadding - 6, 180, this._enabledMetrics.length * intervalPx + 10);
    for (var i = 0; i < this._enabledMetrics.length; ++i) {
      var metricName = this._enabledMetrics[i];
      var info = this._getMetricInfo(metricName);
      ctx.fillStyle = textColor;
      ctx.fillText(info.title || metricName, leftPadding + 26, i * intervalPx + topPadding + 12);
      ctx.fillStyle = info.color || 'grey';
      ctx.fillRect(leftPadding + 10, i * intervalPx + topPadding + 2, 10, 10);
      if (this._metricsBuffer.length) {
        var format = info.format || (value => numberFormat.format(value));
        var value = this._metricsBuffer.peekLast().metrics.get(metricName) || 0;
        ctx.fillText(format(value), leftPadding + 130, i * intervalPx + topPadding + 12);
      }
    }
  }

  /**
   * @override
   */
  onResize() {
    super.onResize();
    this._width = this._canvas.offsetWidth * window.devicePixelRatio;
    this._height = this._canvas.offsetHeight * window.devicePixelRatio;
    this._canvas.width = this._width;
    this._canvas.height = this._height;
    this._update();
  }
};


/** @enum {symbol} */
Timeline.PerformanceMonitor.Mode = {
  Cumulative: Symbol('Cumulative'),
  CumulativeTime: Symbol('CumulativeTime')
};

/**
 * @typedef {!{
 *   title: string,
 *   color: string,
 *   mode: (!Timeline.PerformanceMonitor.Mode|undefined),
 *   min: (number|undefined),
 *   max: (number|undefined),
 *   format: (function(number):string|undefined)
 * }}
 */
Timeline.PerformanceMonitor.Info;
