// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @implements {UI.ListWidget.Delegate<Network.BlockedURLsPane.BlockPattern>}
 */
Network.BlockedURLsPane = class extends UI.VBox {
  constructor() {
    super(true);
    this.registerRequiredCSS('network/blockedURLsPane.css');

    Network.BlockedURLsPane._instance = this;

    this._requestBlockingEnabled = false;
    /** @type {!Array<!Network.BlockedURLsPane.BlockPattern>} */
    this._patternEntries = [];
    this._blockedRequestsInterceptor = new Network.BlockRequestInterceptor();

    this._toolbar = new UI.Toolbar('', this.contentElement);
    this._enabledCheckbox =
        new UI.ToolbarCheckbox(Common.UIString('Enable request blocking'), undefined, this._toggleEnabled.bind(this));
    this._toolbar.appendToolbarItem(this._enabledCheckbox);
    this._toolbar.appendSeparator();
    var addButton = new UI.ToolbarButton(Common.UIString('Add pattern'), 'largeicon-add');
    addButton.addEventListener(UI.ToolbarButton.Events.Click, this._addButtonClicked, this);
    this._toolbar.appendToolbarItem(addButton);
    var clearButton = new UI.ToolbarButton(Common.UIString('Remove all patterns'), 'largeicon-clear');
    clearButton.addEventListener(UI.ToolbarButton.Events.Click, this._removeAll, this);
    this._toolbar.appendToolbarItem(clearButton);

    /** @type {!UI.ListWidget<!Network.BlockedURLsPane.BlockPattern>} */
    this._list = new UI.ListWidget(this);
    this._list.element.classList.add('blocked-urls');
    this._list.registerRequiredCSS('network/blockedURLsPane.css');
    this._list.setEmptyPlaceholder(this._createEmptyPlaceholder());
    this._list.show(this.contentElement);

    /** @type {?UI.ListWidget.Editor<!Network.BlockedURLsPane.BlockPattern>} */
    this._editor = null;

    SDK.targetManager.addModelListener(
        SDK.NetworkManager, SDK.NetworkManager.Events.RequestFinished, this._onRequestFinished, this);

    this._updateThrottler = new Common.Throttler(200);

    this._update();
  }

  /**
   * @return {!Element}
   */
  _createEmptyPlaceholder() {
    var element = this.contentElement.createChild('div', 'no-blocked-urls');
    element.createChild('span').textContent = Common.UIString('Requests are not blocked. ');
    var addLink = element.createChild('span', 'link');
    addLink.textContent = Common.UIString('Add pattern.');
    addLink.href = '';
    addLink.addEventListener('click', this._addButtonClicked.bind(this), false);
    return element;
  }

  /**
   * @param {string} pattern
   * @param {boolean=} shouldShow
   * @return {!Promise}
   */
  static async addPattern(pattern, shouldShow) {
    if (shouldShow)
      await UI.viewManager.showView('network.blocked-urls');
    else
      UI.viewManager.materializedWidget('network.blocked-urls');
    var pane = /** @type {!Network.BlockedURLsPane} */ (Network.BlockedURLsPane._instance);
    var patternEntry = pane._patternEntries.find(patternEntry => patternEntry.pattern === pattern);
    if (patternEntry)
      return;
    pane._patternEntries.push({pattern: pattern, blockedCount: 0, enabled: true});
    pane._requestBlockingEnabled = true;
    return pane._update();
  }

  /**
   * @param {string} pattern
   * @param {boolean=} shouldShow
   * @return {!Promise}
   */
  static async removePattern(pattern, shouldShow) {
    if (shouldShow)
      await UI.viewManager.showView('network.blocked-urls');
    else
      UI.viewManager.materializedWidget('network.blocked-urls');
    var pane = /** @type {!Network.BlockedURLsPane} */ (Network.BlockedURLsPane._instance);
    pane._patternEntries = pane._patternEntries.filter(patternEntry => patternEntry.pattern !== pattern);
    return pane._update();
  }

  /**
   * @return {!Array<!Network.BlockedURLsPane.BlockPattern>}
   */
  static patterns() {
    if (!Network.BlockedURLsPane._instance)
      return [];
    return Network.BlockedURLsPane._instance._patternEntries;
  }

  static reset() {
    if (Network.BlockedURLsPane._instance)
      Network.BlockedURLsPane._instance.reset();
  }

  _addButtonClicked() {
    this._requestBlockingEnabled = true;
    this._update();
    this._list.addNewItem(0, {pattern: '', blockedCount: 0, enabled: true});
  }

  /**
   * @override
   * @param {!Network.BlockedURLsPane.BlockPattern} patternEntry
   * @param {boolean} editable
   * @return {!Element}
   */
  renderItem(patternEntry, editable) {
    var element = createElementWithClass('div', 'blocked-url');
    var checkbox = element.createChild('input', 'blocked-url-checkbox');
    checkbox.type = 'checkbox';
    checkbox.checked = patternEntry.enabled;
    checkbox.disabled = !this._requestBlockingEnabled;
    element.createChild('div', 'blocked-url-label').textContent = patternEntry.pattern;
    element.createChild('div', 'blocked-url-count').textContent =
        Common.UIString('%d blocked', patternEntry.blockedCount);
    element.addEventListener('click', this._togglePattern.bind(this, patternEntry), false);
    checkbox.addEventListener('click', this._togglePattern.bind(this, patternEntry), false);
    return element;
  }

  /**
   * @param {!Network.BlockedURLsPane.BlockPattern} patternEntry
   * @param {!Event} event
   */
  _togglePattern(patternEntry, event) {
    event.consume(true);
    patternEntry.enabled = !patternEntry.enabled;
    this._update();
  }

  _toggleEnabled() {
    this._requestBlockingEnabled = !this._requestBlockingEnabled;
    this._update();
  }

  /**
   * @override
   * @param {!Network.BlockedURLsPane.BlockPattern} patternEntry
   * @param {number} index
   */
  removeItemRequested(patternEntry, index) {
    this._patternEntries.splice(index, 1);
    this._update();
  }

  /**
   * @override
   * @param {!Network.BlockedURLsPane.BlockPattern} patternEntry
   * @return {!UI.ListWidget.Editor}
   */
  beginEdit(patternEntry) {
    this._editor = this._createEditor();
    this._editor.control('url').value = patternEntry.pattern;
    return this._editor;
  }

  /**
   * @override
   * @param {!Network.BlockedURLsPane.BlockPattern} patternEntry
   * @param {!UI.ListWidget.Editor} editor
   * @param {boolean} isNew
   */
  commitEdit(patternEntry, editor, isNew) {
    var pattern = editor.control('url').value;
    patternEntry.pattern = pattern;
    if (isNew)
      this._patternEntries.push(patternEntry);
    this._update();
  }

  /**
   * @return {!UI.ListWidget.Editor<!Network.BlockedURLsPane.BlockPattern>}
   */
  _createEditor() {
    if (this._editor)
      return this._editor;

    var editor = new UI.ListWidget.Editor();
    var content = editor.contentElement();
    var titles = content.createChild('div', 'blocked-url-edit-row');
    titles.createChild('div').textContent =
        Common.UIString('Text pattern to block matching requests; use * for wildcard');
    var fields = content.createChild('div', 'blocked-url-edit-row');
    var urlInput = editor.createInput(
        'url', 'text', '',
        (item, index, input) =>
            !!input.value && !this._patternEntries.find(pattern => pattern.pattern === input.value));
    fields.createChild('div', 'blocked-url-edit-value').appendChild(urlInput);
    return editor;
  }

  _removeAll() {
    this._patternEntries = [];
    this._update();
  }

  /**
   * @return {!Promise}
   */
  _update() {
    var patterns = new Set();
    this._list.clear();
    for (var patternEntry of this._patternEntries) {
      this._list.appendItem(patternEntry, true);
      if (this._requestBlockingEnabled && patternEntry.enabled)
        patterns.add(patternEntry.pattern);
    }

    this._list.element.classList.toggle(
        'blocking-disabled', !this._requestBlockingEnabled && !!this._patternEntries.length);
    this._enabledCheckbox.setChecked(this._requestBlockingEnabled);

    this._blockedRequestsInterceptor.release();
    this._blockedRequestsInterceptor = new Network.BlockRequestInterceptor(patterns);
    return this._blockedRequestsInterceptor.ready();
  }

  reset() {
    for (var patternEntry of this._patternEntries)
      patternEntry.blockedCount = 0;
    this._updateThrottler.schedule(this._update.bind(this));
  }

  /**
   * @param {!Common.Event} event
   */
  _onRequestFinished(event) {
    var request = /** @type {!SDK.NetworkRequest} */ (event.data);
    if (!request.failed)
      return;
    for (var patternEntry of this._patternEntries) {
      if (SDK.RequestInterceptor.patternMatchesUrl(patternEntry.pattern, request.url()))
        patternEntry.blockedCount++;
    }
    this._updateThrottler.schedule(this._update.bind(this));
  }
};

/** @typedef {!{pattern: string, blockedCount: number, enabled: boolean}} */
Network.BlockedURLsPane.BlockPattern;

Network.BlockRequestInterceptor = class extends SDK.RequestInterceptor {
  /**
   * @override
   * @param {!SDK.InterceptionRequest} interceptionRequest
   */
  handle(interceptionRequest) {
    interceptionRequest.continueRequestWithError(Protocol.Network.ErrorReason.BlockedByClient);
  }
};

/** @type {?Network.BlockedURLsPane} */
Network.BlockedURLsPane._instance = null;
