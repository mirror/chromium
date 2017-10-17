// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Console.ConsoleSidebar = class extends UI.VBox {
  constructor() {
    super(true);
    this.setMinimumSize(125, 0);
    this._enabled = Runtime.experiments.isEnabled('logManagement');

    this._tree = new UI.TreeOutlineInShadow();
    this._tree.registerRequiredCSS('console/consoleSidebar.css');
    this._tree.addEventListener(UI.TreeOutline.Events.ElementSelected, this._selectionChanged.bind(this));
    this.contentElement.appendChild(this._tree.element);
    /** @type {?UI.TreeElement} */
    this._selectedTreeElement = null;

    var Levels = ConsoleModel.ConsoleMessage.MessageLevel;
    var filters = [
      new Console.ConsoleFilter(Common.UIString('message'), [], null, Console.ConsoleFilter.allLevelsFilterValue()),
      new Console.ConsoleFilter(
          Common.UIString('error'), [], null, Console.ConsoleFilter.singleLevelMask(Levels.Error)),
      new Console.ConsoleFilter(
          Common.UIString('warning'), [], null, Console.ConsoleFilter.singleLevelMask(Levels.Warning)),
      new Console.ConsoleFilter(Common.UIString('info'), [], null, Console.ConsoleFilter.singleLevelMask(Levels.Info)),
      new Console.ConsoleFilter(
          Common.UIString('verbose'), [], null, Console.ConsoleFilter.singleLevelMask(Levels.Verbose))
    ];
    /** @type {!Array<!Console.ConsoleSidebar.FilterTreeElement>} */
    this._treeElements = [];
    for (var filter of filters) {
      var treeElement = new Console.ConsoleSidebar.FilterTreeElement(filter);
      this._tree.appendChild(treeElement);
      this._treeElements.push(treeElement);
    }
    this._treeElements[0].expand();
    this._treeElements[0].select();
  }

  clear() {
    if (!this._enabled)
      return;
    this._treeElements.forEach(treeElement => treeElement.clear());
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   */
  onMessageAdded(viewMessage) {
    if (!this._enabled)
      return;
    this._treeElements.forEach(treeElement => treeElement.onMessageAdded(viewMessage));
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   * @return {boolean}
   */
  shouldBeVisible(viewMessage) {
    if (!this._enabled || !this._selectedTreeElement)
      return true;
    return this._selectedTreeElement._filter.shouldBeVisible(viewMessage);
  }

  /**
   * @param {!Common.Event} event
   */
  _selectionChanged(event) {
    this._selectedTreeElement = /** @type {!UI.TreeElement} */ (event.data);
    this.dispatchEventToListeners(Console.ConsoleSidebar.Events.FilterSelected);
  }
};

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  FilterSelected: Symbol('FilterSelected')
};

Console.ConsoleSidebar.URLGroupTreeElement = class extends UI.TreeElement {
  /**
   * @param {!Console.ConsoleFilter} filter
   * @param {string=} tooltip
   */
  constructor(filter, tooltip) {
    super(filter.name);
    this._filter = filter;
    if (tooltip)
      this.tooltip = tooltip;
  }
};

Console.ConsoleSidebar.FilterTreeElement = class extends UI.TreeElement {
  /**
   * @param {!Console.ConsoleFilter} filter
   */
  constructor(filter) {
    super(filter.name);
    this._filter = filter;
    this.setExpandable(true);
    /** @type {!Map<string, !Console.ConsoleSidebar.URLGroupTreeElement>} */
    this._urlTreeElements = new Map();
  }

  clear() {
    this._urlTreeElements.clear();
    this.removeChildren();
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   */
  onMessageAdded(viewMessage) {
    if (!this._filter.shouldBeVisible(viewMessage))
      return;
    this._filter.incrementCounters(viewMessage.consoleMessage());
    var url = viewMessage.consoleMessage().url;
    if (!url)
      return;

    var child = this._urlTreeElements.get(url);
    if (!child) {
      var filter = this._filter.clone();
      var parsedURL = url.asParsedURL();
      filter.name = parsedURL ? parsedURL.displayName : url;
      filter.parsedFilters = [{key: Console.ConsoleFilter.FilterType.Url, text: url, negative: false}];
      child = new Console.ConsoleSidebar.URLGroupTreeElement(filter, url);
      this._urlTreeElements.set(url, child);
      this.appendChild(child);
    }
    if (child._filter.shouldBeVisible(viewMessage))
      child._filter.incrementCounters(viewMessage.consoleMessage());
  }
};
