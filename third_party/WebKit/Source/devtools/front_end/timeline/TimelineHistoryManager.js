// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Timeline.TimelineHistoryManager = class {
  constructor() {
    /** @type {!Array<!Timeline.PerformanceModel>} */
    this._recordings = [];
    this._action = UI.actionRegistry.action('timeline.show-history');
    this._action.setEnabled(false);
    /** @type {!Map<string, number>} */
    this._nextNumberByDomain = new Map();
    this._button = new Timeline.TimelineHistoryManager.ToolbarButton(this._action);
    this._button.setVisible(false);

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
    this._button.setText(this._title(performanceModel));
    this._updateState();
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
    this._updateState();
  }

  button() {
    return this._button;
  }

  clear() {
    this._recordings.forEach(model => model.dispose());
    this._recordings = [];
    this._updateState();
    this._nextNumberByDomain.clear();
  }

  /**
   * @return {!Promise<?Timeline.PerformanceModel>}
   */
  async showHistoryDialog() {
    if (this._recordings.length < 2 || !this._enabled)
      return null;

    var model = await Timeline.TimelineHistoryManager.Dialog.show(this._recordings, this._button.element);
    if (!model)
      return null;
    var index = this._recordings.indexOf(model);
    if (index < 0) {
      console.assert(false, `selected recording not found`);
      return null;
    }
    this._recordings.splice(index, 1);
    this._recordings.unshift(model);
    this._button.setText(this._title(model));
    return model;
  }

  cancelIfShowing() {
    Timeline.TimelineHistoryManager.Dialog.cancelIfShowing();
  }

  _updateState() {
    this._action.setEnabled(this._recordings.length > 1 && this._enabled);
    this._button.setVisible(!!this._recordings.length);
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   * @return {!Element}
   */
  static _previewElement(performanceModel) {
    var data = Timeline.TimelineHistoryManager._dataForModel(performanceModel);
    var startedAt = performanceModel.recordStartTime();
    data.time.textContent =
        startedAt ? Common.UIString('(%s ago)', Timeline.TimelineHistoryManager._coarseAge(startedAt)) : '';
    return data.preview;
  }

  /**
   * @param {number} time
   * @return {string}
   */
  static _coarseAge(time) {
    var seconds = Math.ceil((Date.now() - time) / 1000);
    if (seconds < 50)
      return Common.UIString('%s s', seconds);
    var minutes = Math.ceil(seconds / 60);
    if (minutes < 50)
      return Common.UIString('%s m', minutes);
    var hours = Math.ceil(minutes / 60);
    return Common.UIString('%s h', hours);
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   * @return {string}
   */
  _title(performanceModel) {
    return Timeline.TimelineHistoryManager._dataForModel(performanceModel).title;
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   */
  _buildPreview(performanceModel) {
    var parsedURL = performanceModel.timelineModel().pageURL().asParsedURL();
    var domain = parsedURL ? parsedURL.host : '';
    var sequenceNumber = this._nextNumberByDomain.get(domain) || 0;
    var title = sequenceNumber ? Common.UIString('%s #%d', domain, sequenceNumber) : domain;
    this._nextNumberByDomain.set(domain, sequenceNumber + 1);
    var timeElement = createElement('span');

    var preview = createElementWithClass('div', 'preview-item vbox');
    var data = {preview: preview, title: title, time: timeElement};
    performanceModel[Timeline.TimelineHistoryManager._previewDataSymbol] = data;

    preview.appendChild(this._buildTextDetails(performanceModel, title, timeElement));
    var screenshotAndOverview = preview.createChild('div', 'hbox');
    screenshotAndOverview.appendChild(this._buildScreenshotThumbnail(performanceModel));
    screenshotAndOverview.appendChild(this._buildOverview(performanceModel));
    return data.preview;
  }

  /**
   * @param {!Timeline.PerformanceModel} performanceModel
   * @param {string} title
   * @param {!Element} timeElement
   * @return {!Element}
   */
  _buildTextDetails(performanceModel, title, timeElement) {
    var container = createElementWithClass('div', 'text-details hbox');
    container.createChild('span', 'name').textContent = title;
    var tracingModel = performanceModel.tracingModel();
    var duration = Number.millisToString(tracingModel.maximumRecordTime() - tracingModel.minimumRecordTime(), false);
    var timeContainer = container.createChild('span', 'time');
    timeContainer.appendChild(createTextNode(duration));
    timeContainer.appendChild(timeElement);
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

  /**
   * @param {!Timeline.PerformanceModel} model
   * @return {?Timeline.TimelineHistoryManager.PreviewData}
   */
  static _dataForModel(model) {
    return model[Timeline.TimelineHistoryManager._previewDataSymbol] || null;
  }
};

/** @typedef {!{preview: !Element, time: !Element, title: string}} */
Timeline.TimelineHistoryManager.PreviewData;

Timeline.TimelineHistoryManager._maxRecordings = 5;
Timeline.TimelineHistoryManager._previewWidth = 450;
Timeline.TimelineHistoryManager._previewDataSymbol = Symbol('previewData');

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
    this._glassPane.setAnchorBehavior(UI.GlassPane.AnchorBehavior.PreferBottom);

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
   * @param {!Element} anchor
   * @return {!Promise<?Timeline.PerformanceModel>}
   */
  static show(models, anchor) {
    var dialog = new Timeline.TimelineHistoryManager.Dialog(models);
    return dialog._show(anchor);
  }

  static cancelIfShowing() {
    if (!Timeline.TimelineHistoryManager.Dialog._instance)
      return;
    Timeline.TimelineHistoryManager.Dialog._instance._close(null);
  }

  /**
   * @return {!Promise<?Timeline.PerformanceModel>}
   * @param {!Element} anchor
   */
  _show(anchor) {
    Timeline.TimelineHistoryManager.Dialog._instance = this;
    this._glassPane.setContentAnchorBox(anchor.boxInWindow());
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
    var element = Timeline.TimelineHistoryManager._previewElement(item);
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


Timeline.TimelineHistoryManager.ToolbarButton = class extends UI.ToolbarItem {
  /**
   * @param {!UI.Action} action
   */
  constructor(action) {
    super(createElementWithClass('button', 'dropdown-button'));
    var shadowRoot = UI.createShadowRootWithCoreStyles(this.element, 'timeline/historyToolbarButton.css');

    this._contentElement = shadowRoot.createChild('span', 'content');
    var dropdownArrowIcon = UI.Icon.create('smallicon-triangle-down');
    shadowRoot.appendChild(dropdownArrowIcon);
    this.element.addEventListener('click', () => void action.execute(), false);
    this.setEnabled(action.enabled());
    action.addEventListener(UI.Action.Events.Enabled, data => this.setEnabled(/** @type {boolean} */ (data)));
  }

  /**
   * @param {string} text
   */
  setText(text) {
    this._contentElement.textContent = text;
  }
};
