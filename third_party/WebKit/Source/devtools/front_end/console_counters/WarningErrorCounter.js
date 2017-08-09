// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {UI.ToolbarItem.Provider}
 * @unrestricted
 */
ConsoleCounters.WarningErrorCounter = class {
  /**
   * @param {function():number} errorCount
   * @param {function():number} warningCount
   * @param {!Array<{model: !SDK.AnalysisModel, events: !Array<*>}>} listeners
   * @param {function(!Event=)} onclick
   */
  constructor(errorCount, warningCount, listeners, onclick) {
    ConsoleCounters.WarningErrorCounter._instanceForTest = this;

    this._counter = createElement('div');
    if (onclick)
      this._counter.addEventListener('click', onclick, false);
    this._toolbarItem = new UI.ToolbarItem(this._counter);
    var shadowRoot = UI.createShadowRootWithCoreStyles(this._counter, 'console_counters/errorWarningCounter.css');

    this._errors = this._createItem(shadowRoot, 'smallicon-error');
    this._warnings = this._createItem(shadowRoot, 'smallicon-warning');
    this._titles = [];

    this._errorCount = errorCount;
    this._warningCount = warningCount;

    if (listeners) {
      for (var listener of listeners) {
        for (var event of listener.events)
          listener.model.addEventListener(event, this._update, this);
      }
    }
    this._update();
  }

  /**
   * @param {!Node} shadowRoot
   * @param {string} iconType
   * @return {!{item: !Element, text: !Element}}
   */
  _createItem(shadowRoot, iconType) {
    var item = createElementWithClass('span', 'counter-item');
    var icon = item.createChild('label', '', 'dt-icon-label');
    icon.type = iconType;
    var text = icon.createChild('span');
    shadowRoot.appendChild(item);
    return {item: item, text: text};
  }

  /**
   * @param {!{item: !Element, text: !Element}} item
   * @param {number} count
   * @param {string} title
   */
  _updateItem(item, count, title) {
    item.item.classList.toggle('hidden', !count);
    item.text.textContent = count;
    if (count)
      this._titles.push(title);
  }

  _update() {
    var errors = this._errorCount();
    var warnings = this._warningCount();

    this._titles = [];
    this._toolbarItem.setVisible(!!(errors || warnings));
    this._updateItem(this._errors, errors, Common.UIString(errors === 1 ? '%d error' : '%d errors', errors));
    this._updateItem(
        this._warnings, warnings, Common.UIString(warnings === 1 ? '%d warning' : '%d warnings', warnings));
    this._counter.title = this._titles.join(', ');
    UI.inspectorView.toolbarItemResized();
  }

  /**
   * @override
   * @return {?UI.ToolbarItem}
   */
  item() {
    return this._toolbarItem;
  }
};

/**
 * @implements {UI.ToolbarItem.Provider}
 * @unrestricted
 */
ConsoleCounters.ConsoleWarningErrorCounter = class extends ConsoleCounters.WarningErrorCounter {
  constructor() {
    super(
        ConsoleModel.consoleModel.errors.bind(ConsoleModel.consoleModel),
        ConsoleModel.consoleModel.warnings.bind(ConsoleModel.consoleModel), [{
          model: ConsoleModel.consoleModel,
          events: [
            ConsoleModel.ConsoleModel.Events.ConsoleCleared, ConsoleModel.ConsoleModel.Events.MessageAdded,
            ConsoleModel.ConsoleModel.Events.MessageUpdated
          ]
        }],
        Common.console.show.bind(Common.console));
  }
};

/**
 * @implements {UI.ToolbarItem.Provider}
 * @unrestricted
 */
ConsoleCounters.DOMWarningErrorCounter = class extends ConsoleCounters.WarningErrorCounter {
  /**
   * @param {!SDK.AnalysisModel} analysisModel
   */
  constructor(analysisModel) {
    super(
        () => analysisModel.flagCount('error'), () => analysisModel.flagCount('warning'), [{
          model: analysisModel,
          events: [
            SDK.AnalysisModel.Events.NodeFlagged,
            SDK.AnalysisModel.Events.ClearFlags,
          ]
        }],
        /** @type {function()} */ (UI.viewManager.showView.bind(UI.viewManager, 'elements.domAnalysis')));
  }
};