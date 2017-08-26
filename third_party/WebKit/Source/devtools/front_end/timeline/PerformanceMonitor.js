// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @unrestricted
 */
Timeline.PerformanceMonitor = class extends UI.HBox {
  constructor() {
    super(true);
    this.registerRequiredCSS('timeline/performanceMonitor.css');
    this.contentElement.classList.add('perfmon-pane');
    this._model = SDK.targetManager.mainTarget().model(SDK.PerformanceMetricsModel);
    /** @type {!Array<!{timestamp: number, metrics: !Map<string, number>}>} */
    this._metricsBuffer = [];
    /** @const */
    this._pixelsPerMs = 20 / 1000;
    /** @const */
    this._pollIntervalMs = 500;
    this._controlPane = new Timeline.PerformanceMonitor.ControlPane(this.contentElement);
    this._canvas = /** @type {!HTMLCanvasElement} */ (this.contentElement.createChild('canvas'));
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
      this._draw();
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
  }

  /**
   * @param {!Array<!Protocol.Performance.Metric>} metrics
   */
  _processMetrics(metrics) {
    var metricsMap = new Map();
    var timestamp = Date.now();
    for (var metric of metrics) {
      var info = this._controlPane.metricInfo(metric.name);
      var value;
      if (info.mode === Timeline.PerformanceMonitor.Mode.CumulativeTime) {
        value = info.lastTimestamp ?
            100 * Math.min(1, (metric.value - info.lastValue) * 1000 / (timestamp - info.lastTimestamp)) :
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
    // Multiply by 2 as the pollInterval has some jitter and to have some extra samples if window is resized.
    var maxCount = Math.ceil(millisPerWidth / this._pollIntervalMs * 2);
    if (this._metricsBuffer.length > maxCount * 2)  // Multiply by 2 to have a hysteresis.
      this._metricsBuffer.splice(0, this._metricsBuffer.length - maxCount);
    this._controlPane.updateMetrics(metricsMap);
  }

  _draw() {
    var ctx = /** @type {!CanvasRenderingContext2D} */ (this._canvas.getContext('2d'));
    ctx.save();
    ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
    ctx.clearRect(0, 0, this._width, this._height);
    this._drawGrid(ctx);
    for (var metricName of this._controlPane.metrics())
      this._drawMetric(ctx, metricName);
    ctx.restore();
  }

  /**
   * @param {!CanvasRenderingContext2D} ctx
   */
  _drawGrid(ctx) {
    ctx.font = '10px ' + Host.fontFamily();
    ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
    for (var sec = 0;; ++sec) {
      var x = this._width - sec * this._pixelsPerMs * 1000;
      if (x < 0)
        break;
      ctx.beginPath();
      ctx.moveTo(Math.round(x) + 0.5, 0);
      ctx.lineTo(Math.round(x) + 0.5, this._height);
      if (sec % 5 === 0)
        ctx.fillText(Common.UIString('%d sec', sec), Math.round(x) + 4, 12);
      ctx.strokeStyle = sec % 5 ? 'hsla(0, 0%, 0%, 0.02)' : 'hsla(0, 0%, 0%, 0.08)';
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
    var info = this._controlPane.metricInfo(metricName);
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
    ctx.strokeStyle = info.color;
    ctx.lineWidth = 0.5;
    ctx.stroke();
    ctx.globalAlpha = 0.03;
    ctx.fillStyle = info.color;
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
   * @override
   */
  onResize() {
    super.onResize();
    this._width = this._canvas.offsetWidth;
    this._height = this._canvas.offsetHeight;
    this._canvas.width = Math.round(this._width * window.devicePixelRatio);
    this._canvas.height = Math.round(this._height * window.devicePixelRatio);
    this._draw();
  }
};

/** @enum {symbol} */
Timeline.PerformanceMonitor.Mode = {
  CumulativeTime: Symbol('CumulativeTime')
};

/**
 * @typedef {!{
 *   title: string,
 *   color: string,
 *   mode: (!Timeline.PerformanceMonitor.Mode|undefined),
 *   min: (number|undefined),
 *   max: (number|undefined),
 *   currentMin: (number|undefined),
 *   currentMax: (number|undefined),
 *   format: (function(number):string|undefined)
 * }}
 */
Timeline.PerformanceMonitor.Info;

Timeline.PerformanceMonitor.ControlPane = class {
  /**
   * @param {!Element} parent
   */
  constructor(parent) {
    this.element = parent.createChild('div', 'perfmon-control-pane');

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
      return value.toFixed() + '%';
    }

    this._indicators = new Map();
    for (var metric of this.metrics()) {
      var indicator = new Timeline.PerformanceMonitor.MetricIndicator(this.element, this.metricInfo(metric));
      this._indicators.set(metric, indicator);
    }

    this._addButton = this.element.createChild('div', 'perfmon-indicator perfmon-add-button')
                          .createChild('div', 'perfmon-indicator-title');
    this._addButton.textContent = Common.UIString('+ Add indicator');
  }

  /**
   * @return {!Array<string>}
   */
  metrics() {
    return this._enabledMetrics;
  }

  /**
   * @param {string} name
   * @return {!Timeline.PerformanceMonitor.Info}
   */
  metricInfo(name) {
    if (!this._metricsInfo.has(name))
      this._metricsInfo.set(name, {title: name, color: 'grey'});
    return this._metricsInfo.get(name);
  }

  /**
   * @param {!Map<string, number>} metrics
   */
  updateMetrics(metrics) {
    for (const [name, indicator] of this._indicators) {
      if (metrics.has(name))
        indicator.setValue(metrics.get(name));
    }
  }
};

Timeline.PerformanceMonitor.MetricIndicator = class {
  /**
   * @param {!Element} parent
   * @param {!Timeline.PerformanceMonitor.Info} info
   */
  constructor(parent, info) {
    this._info = info;
    this.element = parent.createChild('div', 'perfmon-indicator');
    this.element.createChild('div', 'perfmon-indicator-title').textContent = info.title;
    this._valueElement = this.element.createChild('div', 'perfmon-indicator-value');
    this._valueElement.style.color = info.color;
    this._closeButton = this.element.createChild('div', 'perfmon-indicator-close', 'dt-close-button');
    this._closeButton.gray = true;
  }

  /**
   * @param {number} value
   */
  setValue(value) {
    var format = this._info.format || (value => new Intl.NumberFormat('en-US').format(value));
    this._valueElement.textContent = format(value);
  }
};
