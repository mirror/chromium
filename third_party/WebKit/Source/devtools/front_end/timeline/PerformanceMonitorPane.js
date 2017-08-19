// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Timeline.PerformanceMonitorPane = class extends UI.VBox {
  constructor() {
    super(true);
    this.registerRequiredCSS('timeline/performanceMonitor.css');
    this._model = SDK.targetManager.mainTarget().model(SDK.PerformanceModel);
    this._canvas = this.contentElement.createChild('canvas');
    /** @type {!Array<!{timestamp: number, metrics: !Timeline.PerformanceMonitorPane.Metrics}>} */
    this._metricsBuffer = [];
    this._pixelsPerSecond = 50;
  }

  /**
   * @override
   */
  wasShown() {
    this._model.enable();
    this._pollTimer = setInterval(() => this._poll(), 100);
    this._canvas.width = this._canvas.offsetWidth * window.devicePixelRatio;
    this._canvas.height = this._canvas.offsetHeight * window.devicePixelRatio;
  }

  /**
   * @override
   */
  willHide() {
    clearInterval(this._pollTimer);
    this._model.disable();
    this._metricsBuffer = [];
  }

  async _poll() {
    var metrics = await this._model.requestMetrics();
    console.log(metrics.find(m => m.name === 'ScriptDuration'));
    this._metricsBuffer.push({timestamp: Date.now(), metrics});
    this._draw(500, 300);
  }

  _draw(width, height) {
    var min = Infinity;
    var max = -Infinity;
    for (var metrics of this._metricsBuffer) {
      var metric = Timeline.PerformanceMonitorPane._metricByName(metrics.metrics, 'ScriptDuration');
      min = Math.min(min, metric.value);
      max = Math.max(max, metric.value);
    }
    console.log(min, max);

    var ctx = this._canvas.getContext('2d');
    ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
    ctx.clearRect(0, 0, width, height);
    var x = 10;
    var first = true;
    for (var metrics of this._metricsBuffer) {
      var metric = Timeline.PerformanceMonitorPane._metricByName(metrics.metrics, 'ScriptDuration');
      var y = 10 + height - height * (metric.value - min) / (max - min);
      if (first)
        ctx.moveTo(x + 0.5, y + 0.5);
      else
        ctx.lineTo(x + 0.5, y + 0.5);
      first = false;
      x += 5;
    }
    ctx.strokeStyle = 'blue';
    ctx.strokeWidth = 1;
    ctx.stroke();
  }

  onResize() {
    console.log('resize');
  }

  static _metricByName(metrics, name) {
    return metrics.find(m => m.name === name);
  }

  _animate() {
    this._draw();
    this._animationId = this.contentElement.window().requestAnimationFrame(animate);
  }
};

/** @typedef {!Array<!Protocol.Performance.Metric>} */
Timeline.PerformanceMonitorPane.Metrics;
