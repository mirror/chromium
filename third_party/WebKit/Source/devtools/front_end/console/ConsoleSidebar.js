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
    /** @type {?Console.SidebarItem} */
    this._selectedTreeElement = null;

    var Levels = ConsoleModel.ConsoleMessage.MessageLevel;
    var filters = [
      new Console.ConsoleFilter(Common.UIString('message'), [], null, Console.ConsoleFilter.allLevelsFilterValue()),
      new Console.ConsoleFilter(Levels.Error, [], null, Console.ConsoleFilter.singleLevelMask(Levels.Error)),
      new Console.ConsoleFilter(Levels.Warning, [], null, Console.ConsoleFilter.singleLevelMask(Levels.Warning)),
      new Console.ConsoleFilter(Levels.Info, [], null, Console.ConsoleFilter.singleLevelMask(Levels.Info)),
      new Console.ConsoleFilter(Levels.Verbose, [], null, Console.ConsoleFilter.singleLevelMask(Levels.Verbose))
    ];
    this._groups = filters.map(filter => new Console.SidebarLevelByUrlGroup(filter));
    this._groups.forEach(group => this._tree.appendChild(group));
    this._groups[0].expand();
    this._groups[0].select();
  }

  /**
   * @override
   */
  wasShown() {
    this._tree.focus();
  }

  /**
   * @param {!ConsoleModel.ConsoleMessage} message
   */
  onMessageAdded(message) {
    if (!this._enabled)
      return;
    this._groups.forEach(group => group.onMessageAdded(message));
  }

  resetCounters() {
    if (!this._enabled)
      return;
    this._groups.forEach(group => group.resetCounters());
  }

  clear() {
    if (!this._enabled)
      return;
    this._groups.forEach(group => group.clear());
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   * @return {boolean}
   */
  applyFilters(viewMessage) {
    if (!this._enabled)
      return true;
    this._groups.forEach(group => group.applyFilter(viewMessage));
    if (this._selectedTreeElement)
      return this._selectedTreeElement.shouldBeVisible(viewMessage);
    return true;
  }

  /**
   * @param {!Common.Event} event
   */
  _selectionChanged(event) {
    this._selectedTreeElement = /** @type {!Console.SidebarItem} */ (event.data);
    this.dispatchEventToListeners(Console.ConsoleSidebar.Events.FilterSelected);
  }
};

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  FilterSelected: Symbol('FilterSelected')
};

Console.SidebarItem = class extends UI.TreeElement {
  /**
   * @param {!Console.ConsoleFilter} filter
   * @param {string=} tooltip
   */
  constructor(filter, tooltip) {
    super(filter.name);
    this._filter = filter;
    if (tooltip)
      this.tooltip = tooltip;
    this._countElement = this.listItemElement.createChild('span');
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   */
  applyFilter(viewMessage) {
    this._filter.applyFilter(viewMessage);
    this._updateCounters();
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   * @return {boolean}
   */
  shouldBeVisible(viewMessage) {
    return this._filter.shouldBeVisible(viewMessage);
  }

  _updateCounters() {
    var totalCount = this._filter.errorCount + this._filter.warningCount + this._filter.infoCount;
    this._countElement.textContent = totalCount > 0 ? totalCount : '';
  }

  resetCounters() {
    this._filter.resetCounters();
    this._updateCounters();
  }
};

Console.SidebarLevelByUrlGroup = class extends Console.SidebarItem {
  /**
   * @param {!Console.ConsoleFilter} filter
   */
  constructor(filter) {
    super(filter);
    this.setExpandable(true);
    /** @type {!Map<string, !Console.SidebarItem>} */
    this._urlItems = new Map();
    this._updateCounters();
  }

  /**
   * @override
   */
  _updateCounters() {
    var totalCount = this._filter.errorCount + this._filter.warningCount + this._filter.infoCount;
    this._countElement.textContent = totalCount > 0 ? totalCount : '';

    var titleText = `${totalCount ? totalCount : 'No'} ${this._filter.name}`;
    if (this._filter.name !== ConsoleModel.ConsoleMessage.MessageLevel.Info &&
        this._filter.name !== ConsoleModel.ConsoleMessage.MessageLevel.Verbose)
      titleText += `${totalCount !== 1 ? 's' : ''}`;
    this.title = titleText;
  }

  /**
   * @override
   * @param {!Console.ConsoleViewMessage} viewMessage
   */
  applyFilter(viewMessage) {
    var message = viewMessage.consoleMessage();
    var childMatch = this._urlItems.get(/** @type {string} */ (message.url));
    if (childMatch)
      childMatch.applyFilter(viewMessage);

    this._filter.applyFilter(viewMessage);
    this._updateCounters();
  }

  /**
   * @override
   */
  resetCounters() {
    this._filter.resetCounters();
    this._updateCounters();
    for (var child of this.children())
      child.resetCounters();
  }

  /**
   * @param {!ConsoleModel.ConsoleMessage} message
   */
  onMessageAdded(message) {
    var url = message.url;
    if (!url || !this._filter.levelsMask[/** @type {string} */ (message.level)] || this._urlItems.get(url))
      return;

    var parsedFilter = {key: Console.ConsoleFilter.FilterType.Url, text: url, negative: false};
    var levelsMask = {};
    for (var level of Object.keys(this._filter.levelsMask))
      levelsMask[level] = this._filter.levelsMask[level];
    var parsedURL = url.asParsedURL();
    var filter = new Console.ConsoleFilter(parsedURL ? parsedURL.displayName : url, [parsedFilter], null, levelsMask);
    var child = new Console.SidebarItem(filter, url);
    this._urlItems.set(url, child);
    this.appendChild(child);
  }

  clear() {
    this._urlItems.clear();
    this.removeChildren();
  }
};
