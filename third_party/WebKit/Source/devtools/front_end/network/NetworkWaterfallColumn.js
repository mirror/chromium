// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
Network.NetworkWaterfallColumn = class extends UI.VBox {
  /**
   * @param {!Network.NetworkTimeCalculator} calculator
   */
  constructor(calculator) {
    super(false);
    this.registerRequiredCSS('network/networkWaterfallColumn.css');

    this._canvas = this.contentElement.createChild('canvas');
    this._canvas.tabIndex = 1;
    this.setDefaultFocusedElement(this._canvas);

    /** @const */
    this._leftPadding = 5;
    /** @const */
    this._fontSize = 10;

    this._calculator = calculator;

    this._rawRowHeight = 0;
    this._rowHeight = 0;

    this._offsetWidth = 0;
    this._offsetHeight = 0;
    this._startTime = this._calculator.minimumBoundary();
    this._endTime = this._calculator.maximumBoundary();

    /** @type {!Array<!Network.NetworkNode>} */
    this._nodes = [];

    /** @type {?PerfUI.TimelineGrid.DividersData} */
    this._dividersData = null;
    /** @type {!Map<string, !Array<number>>} */
    this._eventDividers = new Map();

    /** @type {(number|undefined)} */
    this._updateRequestID;

    this._styleForTimeRangeName = Network.NetworkWaterfallColumn._buildRequestTimeRangeStyle();

    var resourceStyleTuple = Network.NetworkWaterfallColumn._buildResourceTypeStyle();
    /** @type {!Map<!Common.ResourceType, !Network.NetworkWaterfallColumn._LayerStyle>} */
    this._styleForWaitingResourceType = resourceStyleTuple[0];
    /** @type {!Map<!Common.ResourceType, !Network.NetworkWaterfallColumn._LayerStyle>} */
    this._styleForDownloadingResourceType = resourceStyleTuple[1];

    var baseLineColor = UI.themeSupport.patchColorText('#a5a5a5', UI.ThemeSupport.ColorUsage.Foreground);
    /** @type {!Network.NetworkWaterfallColumn._LayerStyle} */
    this._wiskerStyle = {borderColor: baseLineColor, lineWidth: 1};
    /** @type {!Network.NetworkWaterfallColumn._LayerStyle} */
    this._hoverDetailsStyle = {fillStyle: baseLineColor, lineWidth: 1, borderColor: baseLineColor};

    /** @type {!Map<!Network.NetworkWaterfallColumn._LayerStyle, !Path2D>} */
    this._pathForStyle = new Map();
    /** @type {!Array<!Network.NetworkWaterfallColumn._TextLayer>} */
    this._textLayers = [];
  }

  /**
   * @return {!Map<!Network.RequestTimeRangeNames, !Network.NetworkWaterfallColumn._LayerStyle>}
   */
  static _buildRequestTimeRangeStyle() {
    const types = Network.RequestTimeRangeNames;
    var styleMap = new Map();
    styleMap.set(types.Connecting, {fillStyle: '#FF9800'});
    styleMap.set(types.SSL, {fillStyle: '#9C27B0'});
    styleMap.set(types.DNS, {fillStyle: '#009688'});
    styleMap.set(types.Proxy, {fillStyle: '#A1887F'});
    styleMap.set(types.Blocking, {fillStyle: '#AAAAAA'});
    styleMap.set(types.Push, {fillStyle: '#8CDBff'});
    styleMap.set(types.Queueing, {fillStyle: 'white', lineWidth: 2, borderColor: 'lightgrey'});
    // This ensures we always show at least 2 px for a request.
    styleMap.set(types.Receiving, {fillStyle: '#03A9F4', lineWidth: 2, borderColor: '#03A9F4'});
    styleMap.set(types.Waiting, {fillStyle: '#00C853'});
    styleMap.set(types.ReceivingPush, {fillStyle: '#03A9F4'});
    styleMap.set(types.ServiceWorker, {fillStyle: 'orange'});
    styleMap.set(types.ServiceWorkerPreparation, {fillStyle: 'orange'});
    return styleMap;
  }

  /**
   * @return {!Array<!Map<!Common.ResourceType, !Network.NetworkWaterfallColumn._LayerStyle>>}
   */
  static _buildResourceTypeStyle() {
    const baseResourceTypeColors = new Map([
      ['document', 'hsl(215, 100%, 80%)'],
      ['font', 'hsl(8, 100%, 80%)'],
      ['media', 'hsl(90, 50%, 80%)'],
      ['image', 'hsl(90, 50%, 80%)'],
      ['script', 'hsl(31, 100%, 80%)'],
      ['stylesheet', 'hsl(272, 64%, 80%)'],
      ['texttrack', 'hsl(8, 100%, 80%)'],
      ['websocket', 'hsl(0, 0%, 95%)'],
      ['xhr', 'hsl(53, 100%, 80%)'],
      ['fetch', 'hsl(53, 100%, 80%)'],
      ['other', 'hsl(0, 0%, 95%)'],
    ]);
    var waitingStyleMap = new Map();
    var downloadingStyleMap = new Map();

    for (var resourceType of Object.values(Common.resourceTypes)) {
      var color = baseResourceTypeColors.get(resourceType.name());
      if (!color)
        color = baseResourceTypeColors.get('other');
      var borderColor = toBorderColor(color);

      waitingStyleMap.set(resourceType, {fillStyle: toWaitingColor(color), lineWidth: 1, borderColor: borderColor});
      downloadingStyleMap.set(resourceType, {fillStyle: color, lineWidth: 1, borderColor: borderColor});
    }
    return [waitingStyleMap, downloadingStyleMap];

    /**
     * @param {string} color
     */
    function toBorderColor(color) {
      var parsedColor = Common.Color.parse(color);
      var hsla = parsedColor.hsla();
      hsla[1] /= 2;
      hsla[2] -= Math.min(hsla[2], 0.2);
      return parsedColor.asString(null);
    }

    /**
     * @param {string} color
     */
    function toWaitingColor(color) {
      var parsedColor = Common.Color.parse(color);
      var hsla = parsedColor.hsla();
      hsla[2] *= 1.1;
      return parsedColor.asString(null);
    }
  }

  _resetPaths() {
    this._pathForStyle.clear();
    this._pathForStyle.set(this._wiskerStyle, new Path2D());
    this._styleForTimeRangeName.forEach(style => this._pathForStyle.set(style, new Path2D()));
    this._styleForWaitingResourceType.forEach(style => this._pathForStyle.set(style, new Path2D()));
    this._styleForDownloadingResourceType.forEach(style => this._pathForStyle.set(style, new Path2D()));
    this._pathForStyle.set(this._hoverDetailsStyle, new Path2D());
  }

  /**
   * @param {!SDK.NetworkRequest} request
   * @param {number} mouseOffsetX
   * @param {number} mouseOffsetY
   * @return {?UI.PopoverRequest}
   */
  getPopoverRequest(request, mouseOffsetX, mouseOffsetY) {
    var index = this._nodes.findIndex(node => node.request() === request);
    if (index === -1)
      return null;
    var offsetTop = this._rowHeight * index;
    var useTimingBars = !Common.moduleSetting('networkColorCodeResourceTypes').get() && !this._calculator.startAtZero;
    if (useTimingBars) {
      var range = Network.RequestTimingView.calculateRequestTimeRanges(request, 0)
                      .find(data => data.name === Network.RequestTimeRangeNames.Total);
      var start = this._timeToPosition(range.start);
      var end = this._timeToPosition(range.end);
    } else {
      var range = this._getSimplifiedBarRange(request, 0);
      var start = range.start;
      var end = range.end;
    }

    if (end - start < 50) {
      var halfWidth = (end - start) / 2;
      start = start + halfWidth - 25;
      end = end - halfWidth + 25;
    }

    if (mouseOffsetX < start || mouseOffsetX > end)
      return null;

    var barHeight = this._getBarHeight(range.name);
    var barTop = (this._rowHeight - barHeight) / 2;

    if (mouseOffsetY < barTop || mouseOffsetY > barTop + barHeight)
      return null;

    var anchorBox = this.element.boxInWindow();
    anchorBox.x += start;
    anchorBox.y += offsetTop + barTop;
    anchorBox.width = end - start;
    anchorBox.height = barHeight;

    return {
      box: anchorBox,
      show: popover => {
        var content =
            Network.RequestTimingView.createTimingTable(/** @type {!SDK.NetworkRequest} */ (request), this._calculator);
        popover.contentElement.appendChild(content);
        return Promise.resolve(true);
      }
    };
  }

  /**
   * @param {number} height
   */
  setRowHeight(height) {
    this._rawRowHeight = height;
    this._updateRowHeight();
  }

  _updateRowHeight() {
    this._rowHeight = Math.floor(this._rawRowHeight * window.devicePixelRatio) / window.devicePixelRatio;
  }

  /**
   * @param {!Network.NetworkTimeCalculator} calculator
   */
  setCalculator(calculator) {
    this._calculator = calculator;
  }

  scheduleDraw() {
    if (this._updateRequestID)
      return;
    this._updateRequestID = this.element.window().requestAnimationFrame(() => this._innerUpdate());
  }

  /**
   * @param {!PerfUI.TimelineGrid.DividersData} dividersData
   */
  setDividersData(dividersData) {
    this._dividersData = dividersData;
  }
  /**
   * @param {!Array<!Network.NetworkNode>=} nodes
   * @param {!Map<string, !Array<number>>=} eventDividers
   */
  update(nodes, eventDividers) {
    if (nodes)
      this._nodes = nodes;

    if (eventDividers !== undefined)
      this._eventDividers = eventDividers;
    this._innerUpdate();
  }

  _innerUpdate() {
    if (!this.isShowing())
      return;
    if (this._updateRequestID) {
      this.element.window().cancelAnimationFrame(this._updateRequestID);
      delete this._updateRequestID;
    }
    this._startTime = this._calculator.minimumBoundary();
    this._endTime = this._calculator.maximumBoundary();
    this._resetCanvas();
    this._resetPaths();
    this._textLayers = [];
    this._draw();
  }

  _resetCanvas() {
    var ratio = window.devicePixelRatio;
    this._canvas.width = this._offsetWidth * ratio;
    this._canvas.height = this._offsetHeight * ratio;
    this._canvas.style.width = this._offsetWidth + 'px';
    this._canvas.style.height = this._offsetHeight + 'px';
  }

  /**
   * @override
   */
  onResize() {
    super.onResize();
    this._updateRowHeight();
    this._calculateCanvasSize();
    this.scheduleDraw();
  }

  _calculateCanvasSize() {
    this._offsetWidth = this.contentElement.offsetWidth;
    this._offsetHeight = this.contentElement.offsetHeight;
    this._calculator.setDisplayWidth(this._offsetWidth);
  }

  /**
   * @param {number} time
   * @return {number}
   */
  _timeToPosition(time) {
    var availableWidth = this._offsetWidth - this._leftPadding;
    var timeToPixel = availableWidth / (this._endTime - this._startTime);
    return Math.floor(this._leftPadding + (time - this._startTime) * timeToPixel);
  }

  _didDrawForTest() {
  }

  _draw() {
    var useTimingBars = !Common.moduleSetting('networkColorCodeResourceTypes').get() && !this._calculator.startAtZero;
    var nodes = this._nodes;
    var context = this._canvas.getContext('2d');
    context.save();
    context.scale(window.devicePixelRatio, window.devicePixelRatio);
    context.rect(0, 0, this._offsetWidth, this._offsetHeight);
    context.clip();
    for (var i = 0; i < nodes.length; i++) {
      var rowOffset = this._rowHeight * i;
      var node = nodes[i];
      var drawNodes = [];
      if (node.hasChildren() && !node.expanded)
        drawNodes = /** @type {!Array<!Network.NetworkNode>} */ (node.flatChildren());
      drawNodes.push(node);
      for (var drawNode of drawNodes) {
        if (useTimingBars)
          this._buildTimingBarLayers(drawNode, rowOffset);
        else
          this._buildSimplifiedBarLayers(context, drawNode, rowOffset);
      }
    }
    this._drawLayers(context);

    context.save();
    context.fillStyle = UI.themeSupport.patchColorText('#888', UI.ThemeSupport.ColorUsage.Foreground);
    for (var textData of this._textLayers)
      context.fillText(textData.text, textData.x, textData.y);
    context.restore();

    this._drawEventDividers(context);
    context.restore();

    if (this._dividersData)
      PerfUI.TimelineGrid.drawCanvasGrid(context, this._dividersData);
    this._didDrawForTest();
  }

  /**
   * @param {!CanvasRenderingContext2D} context
   */
  _drawLayers(context) {
    for (var entry of this._pathForStyle) {
      var style = /** @type {!Network.NetworkWaterfallColumn._LayerStyle} */ (entry[0]);
      var path = /** @type {!Path2D} */ (entry[1]);
      context.save();
      context.beginPath();
      if (style.lineWidth) {
        context.lineWidth = style.lineWidth;
        context.strokeStyle = style.borderColor;
        context.stroke(path);
      }
      if (style.fillStyle) {
        context.fillStyle = style.fillStyle;
        context.fill(path);
      }
      context.restore();
    }
  }

  /**
   * @param {!CanvasRenderingContext2D} context
   */
  _drawEventDividers(context) {
    context.save();
    context.lineWidth = 1;
    for (var color of this._eventDividers.keys()) {
      context.strokeStyle = color;
      for (var time of this._eventDividers.get(color)) {
        context.beginPath();
        var x = this._timeToPosition(time);
        context.moveTo(x, 0);
        context.lineTo(x, this._offsetHeight);
      }
      context.stroke();
    }
    context.restore();
  }

  /**
   * @param {!Network.RequestTimeRangeNames=} type
   * @return {number}
   */
  _getBarHeight(type) {
    var types = Network.RequestTimeRangeNames;
    switch (type) {
      case types.Connecting:
      case types.SSL:
      case types.DNS:
      case types.Proxy:
      case types.Blocking:
      case types.Push:
      case types.Queueing:
        return 7;
      default:
        return 13;
    }
  }

  /**
   * @param {!SDK.NetworkRequest} request
   * @param {number} borderOffset
   * @return {!{start: number, mid: number, end: number}}
   */
  _getSimplifiedBarRange(request, borderOffset) {
    var drawWidth = this._offsetWidth - this._leftPadding;
    var percentages = this._calculator.computeBarGraphPercentages(request);
    return {
      start: this._leftPadding + Math.floor((percentages.start / 100) * drawWidth) + borderOffset,
      mid: this._leftPadding + Math.floor((percentages.middle / 100) * drawWidth) + borderOffset,
      end: this._leftPadding + Math.floor((percentages.end / 100) * drawWidth) + borderOffset
    };
  }

  /**
   * @param {!CanvasRenderingContext2D} context
   * @param {!Network.NetworkNode} node
   * @param {number} y
   */
  _buildSimplifiedBarLayers(context, node, y) {
    var request = node.request();
    if (!request)
      return;
    const borderWidth = 1;
    var borderOffset = borderWidth % 2 === 0 ? 0 : 0.5;

    var ranges = this._getSimplifiedBarRange(request, borderOffset);
    var height = this._getBarHeight();
    y += Math.floor(this._rowHeight / 2 - height / 2 + borderWidth) - borderWidth / 2;

    var waitingStyle = this._styleForWaitingResourceType.get(request.resourceType());
    var waitingPath = this._pathForStyle.get(waitingStyle);
    waitingPath.rect(ranges.start, y, ranges.mid - ranges.start, height - borderWidth);

    var barWidth = Math.max(2, ranges.end - ranges.mid);
    var downloadingStyle = this._styleForDownloadingResourceType.get(request.resourceType());
    var downloadingPath = this._pathForStyle.get(downloadingStyle);
    downloadingPath.rect(ranges.mid, y, barWidth, height - borderWidth);

    /** @type {?{left: string, right: string, tooltip: (string|undefined)}} */
    var labels = null;
    if (node.hovered()) {
      labels = this._calculator.computeBarGraphLabels(request);
      const barDotLineLength = 10;
      var leftLabelWidth = context.measureText(labels.left).width;
      var rightLabelWidth = context.measureText(labels.right).width;
      var hoverLinePath = this._pathForStyle.get(this._hoverDetailsStyle);

      if (leftLabelWidth < ranges.mid - ranges.start) {
        var midBarX = ranges.start + (ranges.mid - ranges.start - leftLabelWidth) / 2;
        this._textLayers.push({text: labels.left, x: midBarX, y: y + this._fontSize});
      } else if (barDotLineLength + leftLabelWidth + this._leftPadding < ranges.start) {
        this._textLayers.push(
            {text: labels.left, x: ranges.start - leftLabelWidth - barDotLineLength - 1, y: y + this._fontSize});
        hoverLinePath.moveTo(ranges.start - barDotLineLength, y + Math.floor(height / 2));
        hoverLinePath.arc(ranges.start, y + Math.floor(height / 2), 2, 0, 2 * Math.PI);
        hoverLinePath.moveTo(ranges.start - barDotLineLength, y + Math.floor(height / 2));
        hoverLinePath.lineTo(ranges.start, y + Math.floor(height / 2));
      }

      var endX = ranges.mid + barWidth + borderOffset;
      if (rightLabelWidth < endX - ranges.mid) {
        var midBarX = ranges.mid + (endX - ranges.mid - rightLabelWidth) / 2;
        this._textLayers.push({text: labels.right, x: midBarX, y: y + this._fontSize});
      } else if (endX + barDotLineLength + rightLabelWidth < this._offsetWidth - this._leftPadding) {
        this._textLayers.push({text: labels.right, x: endX + barDotLineLength + 1, y: y + this._fontSize});
        hoverLinePath.moveTo(endX, y + Math.floor(height / 2));
        hoverLinePath.arc(endX, y + Math.floor(height / 2), 2, 0, 2 * Math.PI);
        hoverLinePath.moveTo(endX, y + Math.floor(height / 2));
        hoverLinePath.lineTo(endX + barDotLineLength, y + Math.floor(height / 2));
      }
    }

    if (!this._calculator.startAtZero) {
      var queueingRange = Network.RequestTimingView.calculateRequestTimeRanges(request, 0)
                              .find(data => data.name === Network.RequestTimeRangeNames.Total);
      var leftLabelWidth = labels ? context.measureText(labels.left).width : 0;
      var leftTextPlacedInBar = leftLabelWidth < ranges.mid - ranges.start;
      const wiskerTextPadding = 13;
      var textOffset = (labels && !leftTextPlacedInBar) ? leftLabelWidth + wiskerTextPadding : 0;
      var queueingStart = this._timeToPosition(queueingRange.start);
      if (ranges.start - textOffset > queueingStart) {
        var wiskerPath = this._pathForStyle.get(this._wiskerStyle);
        wiskerPath.moveTo(queueingStart, y + Math.floor(height / 2));
        wiskerPath.lineTo(ranges.start - textOffset, y + Math.floor(height / 2));

        // TODO(allada) This needs to be floored.
        const wiskerHeight = height / 2;
        wiskerPath.moveTo(queueingStart + borderOffset, y + wiskerHeight / 2);
        wiskerPath.lineTo(queueingStart + borderOffset, y + height - wiskerHeight / 2 - 1);
      }
    }
  }

  /**
   * @param {!Network.NetworkNode} node
   * @param {number} y
   */
  _buildTimingBarLayers(node, y) {
    var request = node.request();
    if (!request)
      return;
    var ranges = Network.RequestTimingView.calculateRequestTimeRanges(request, 0);
    for (var range of ranges) {
      if (range.name === Network.RequestTimeRangeNames.Total || range.name === Network.RequestTimeRangeNames.Sending ||
          range.end - range.start === 0)
        continue;

      var style = this._styleForTimeRangeName.get(range.name);
      var path = this._pathForStyle.get(style);
      var lineWidth = style.lineWidth || 0;
      var height = this._getBarHeight(range.name);
      var middleBarY = y + Math.floor(this._rowHeight / 2 - height / 2) + lineWidth / 2;
      var start = this._timeToPosition(range.start);
      var end = this._timeToPosition(range.end);
      path.rect(start, middleBarY, end - start, height - lineWidth);
    }
  }
};

/** @typedef {!{fillStyle: (string|undefined), lineWidth: (number|undefined), borderColor: (string|undefined)}} */
Network.NetworkWaterfallColumn._LayerStyle;

/** @typedef {!{x: number, y: number, text: string}} */
Network.NetworkWaterfallColumn._TextLayer;
