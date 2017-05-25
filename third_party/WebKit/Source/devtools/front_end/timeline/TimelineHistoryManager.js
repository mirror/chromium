// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Timeline.TimelineHistoryManager = class {
  /**
   * @param {!Timeline.TimelineHistoryManager.Client} client
   */
  constructor(client) {
    /** @type {!Array<!Timeline.PerformanceModel>} */
    this._recordings = [];
    this._client = client;
    this._action = UI.actionRegistry.action('timeline.show-history');
    this._action.setEnabled(false);

    this._allOverviews = [
      {constructor: Timeline.TimelineEventOverviewResponsiveness, height: 3},
      {constructor: Timeline.TimelineEventOverviewFrames, height: 16},
      {constructor: Timeline.TimelineEventOverviewCPUActivity, height: 20},
      {constructor: Timeline.TimelineEventOverviewNetwork, height: 8}
    ];
    this._totalHeight = this._allOverviews.reduce((acc, entry) => acc + entry.height, 0);
    this._enabled = true;
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   */
  addRecording(performanceModel) {
    this._recordings.unshift(performanceModel);
    this._buildPreview(performanceModel);
    this._updateAction();
    if (this._recordings.length < Timeline.TimelineHistoryManager._maxRecordings)
      return;
    var lruModel = this._recordings.pop();
    lruModel.dispose();
  }

  /**
   * @param {boolean} enabled
   */
  setEnabled(enabled) {
    this._enabled = enabled;
    this._updateAction();
  }

  _updateAction() {
    this._action.setEnabled(this._recordings.length > 1 && this._enabled);
  }

  clear() {
    this._recordings.forEach(model => model.dispose());
    this._recordings = [];
    this._updateAction();
  }

  /**
   * @param {number} x
   * @param {number} y
   */
  async showHistoryDialog(x, y) {
    if (this._recordings.length < 2 || !this._enabled)
      return;
    var model = await Timeline.TimelineHistoryManager.Dialog.show(this._recordings, x, y);
    if (model)
      this._show(model);
  }

  cancelIfShowing() {
    Timeline.TimelineHistoryManager.Dialog.cancelIfShowing();
  }

  /**
   * @param {!Timeline.PerformanceModel} model
   */
  _show(model) {
    var index = this._recordings.indexOf(model);
    if (index < 0) {
      console.assert(false, `Requested model not found`);
      return;
    }
    this._recordings.splice(index, 1);
    this._recordings.unshift(model);
    this._client.setModel(model);
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   */
  _buildPreview(performanceModel) {
    if (performanceModel[Timeline.TimelineHistoryManager._previewSymbol])
      return;
    var previewItem = createElementWithClass('div', 'preview-item vbox');
    performanceModel[Timeline.TimelineHistoryManager._previewSymbol] = previewItem;
    previewItem.appendChild(this._buildTextDetails(performanceModel));
    var screenshotAndOverview = previewItem.createChild('div', 'hbox');
    screenshotAndOverview.appendChild(this._buildScreenshotThumbnail(performanceModel));
    screenshotAndOverview.appendChild(this._buildOverview(performanceModel));
    return previewItem;
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   * @return {!Element}
   */
  _buildTextDetails(performanceModel) {
    var container = createElementWithClass('div', 'text-details hbox');
    var parsedURL = performanceModel.timelineModel().pageURL().asParsedURL();
    container.createChild('span', 'url').textContent = parsedURL ? parsedURL.host : '-';
    var tracingModel = performanceModel.tracingModel();
    var duration = Number.millisToString(tracingModel.maximumRecordTime() - tracingModel.minimumRecordTime(), false);
    var startedAt = performanceModel.recordStartTime();
    var time = startedAt ? Common.UIString('%s @ %s', duration, new Date(startedAt).toLocaleTimeString()) : duration;
    container.createChild('span', 'time').textContent = time;
    return container;
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   * @return {!Element}
   */
  _buildScreenshotThumbnail(performanceModel) {
    var container = createElementWithClass('div', 'screenshot-thumb');
    container.style.width = this._totalHeight * 3 / 2 + 'px';
    container.style.height = this._totalHeight + 'px';
    var filmStripModel = performanceModel.filmStripModel();
    var lastFrame = filmStripModel.frames().peekLast();
    if (!lastFrame)
      return container;
    lastFrame.imageDataPromise()
        .then(data => UI.loadImageFromData(data))
        .then(image => image && container.appendChild(image));
    return container;
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   * @return {!Element}
   */
  _buildOverview(performanceModel) {
    var container = createElement('div');

    container.style.width = Timeline.TimelineHistoryManager._previewWidth + 'px';
    container.style.height = this._totalHeight + 'px';
    var canvas = container.createChild('canvas');
    canvas.width = window.devicePixelRatio * Timeline.TimelineHistoryManager._previewWidth;
    canvas.height = window.devicePixelRatio * this._totalHeight;

    var ctx = canvas.getContext('2d');
    var yOffset = 0;
    for (var overview of this._allOverviews) {
      var timelineOverview = new overview.constructor();
      timelineOverview.setCanvasSize(Timeline.TimelineHistoryManager._previewWidth, overview.height);
      timelineOverview.setModel(performanceModel);
      timelineOverview.update();
      var sourceContext = timelineOverview.context();
      var imageData = sourceContext.getImageData(0, 0, sourceContext.canvas.width, sourceContext.canvas.height);
      ctx.putImageData(imageData, 0, yOffset);
      yOffset += overview.height;
    }
    return container;
  }
};

Timeline.TimelineHistoryManager._maxRecordings = 5;
Timeline.TimelineHistoryManager._previewWidth = 450;
Timeline.TimelineHistoryManager._previewSymbol = Symbol('previewElement');

/**
 * @interface
 */
Timeline.TimelineHistoryManager.Client = function() {};

Timeline.TimelineHistoryManager.Client.prototype = {
  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   */
  setModel(performanceModel) {}
};


/**
 * @implements {UI.ListDelegate<!Timeline.PerformanceModel>}
 */
Timeline.TimelineHistoryManager.Dialog = class {
  /**
   * @param {!Array<!Timeline.PerformanceModel>} models
   */
  constructor(models) {
    this._glassPane = new UI.GlassPane();
    this._glassPane.setSizeBehavior(UI.GlassPane.SizeBehavior.MeasureContent);
    this._glassPane.setOutsideClickCallback(() => this._close(null));
    this._glassPane.setPointerEventsBehavior(UI.GlassPane.PointerEventsBehavior.BlockedByGlassPane);

    var widget = new UI.Widget(true);
    widget.registerRequiredCSS('timeline/timelineHistoryManager.css');
    widget.show(this._glassPane.contentElement);

    this._listControl = new UI.ListControl(this, UI.ListMode.NonViewport);
    this._listControl.element.addEventListener('mousemove', this._onMouseMove.bind(this), false);
    this._listControl.replaceAllItems(models);
    this._listControl.selectItem(models[0]);

    widget.parentWidget().setDefaultFocusedElement(this._listControl.element);
    widget.contentElement.appendChild(this._listControl.element);
    widget.contentElement.addEventListener('keydown', this._onKeyDown.bind(this), false);
    widget.contentElement.addEventListener('click', this._onClick.bind(this), false);

    /** @type {?function(?Timeline.PerformanceModel)} */
    this._selectionDone = null;
  }

  /**
   * @param {!Array<!Timeline.PerformanceModel>} models
   * @param {number} x
   * @param {number} y
   * @return {!Promise<?Timeline.PerformanceModel>}
   */
  static show(models, x, y) {
    var dialog = new Timeline.TimelineHistoryManager.Dialog(models);
    return dialog._show(x, y);
  }

  static cancelIfShowing() {
    if (!Timeline.TimelineHistoryManager.Dialog._instance)
      return;
    Timeline.TimelineHistoryManager.Dialog._instance._close(null);
  }

  /**
   * @return {!Promise<?Timeline.PerformanceModel>}
   * @param {number} x
   * @param {number} y
   */
  _show(x, y) {
    Timeline.TimelineHistoryManager.Dialog._instance = this;
    this._glassPane.setContentPosition(x, y);
    this._glassPane.show(/** @type {!Document} */ (this._glassPane.contentElement.ownerDocument));
    return new Promise(fulfill => this._selectionDone = fulfill);
  }

  /**
   * @param {!Event} event
   */
  _onMouseMove(event) {
    var node = event.target.enclosingNodeOrSelfWithClass('preview-item');
    var listItem = node && this._listControl.itemForNode(node);
    if (!listItem)
      return;
    this._listControl.selectItem(listItem);
  }

  /**
   * @param {!Event} event
   */
  _onClick(event) {
    if (!event.target.enclosingNodeOrSelfWithClass('preview-item'))
      return;
    this._close(this._listControl.selectedItem());
  }

  /**
   * @param {!Event} event
   */
  _onKeyDown(event) {
    if (event.key !== 'Enter')
      return;
    event.consume(true);
    this._close(this._listControl.selectedItem());
  }

  /**
   * @param {?Timeline.PerformanceModel} model
   */
  _close(model) {
    this._selectionDone(model);
    this._glassPane.hide();
    Timeline.TimelineHistoryManager.Dialog._instance = null;
  }

  /**
   * @override
   * @param {!Timeline.PerformanceModel} item
   * @return {!Element}
   */
  createElementForItem(item) {
    var element = item[Timeline.TimelineHistoryManager._previewSymbol];
    element.classList.remove('selected');
    return element;
  }

  /**
   * This method is not called in NonViewport mode.
   * Return zero to make list measure the item (only works in SameHeight mode).
   * @override
   * @param {!Timeline.PerformanceModel} item
   * @return {number}
   */
  heightForItem(item) {
    console.assert(false, 'Should not be called');
    return 0;
  }

  /**
   * @override
   * @param {!Timeline.PerformanceModel} item
   * @return {boolean}
   */
  isItemSelectable(item) {
    return true;
  }

  /**
   * @override
   * @param {?Timeline.PerformanceModel} from
   * @param {?Timeline.PerformanceModel} to
   * @param {?Element} fromElement
   * @param {?Element} toElement
   */
  selectedItemChanged(from, to, fromElement, toElement) {
    if (fromElement)
      fromElement.classList.remove('selected');
    if (toElement)
      toElement.classList.add('selected');
  }
};

/**
 * @type {?Timeline.TimelineHistoryManager.Dialog}
 */
Timeline.TimelineHistoryManager.Dialog._instance = null;
