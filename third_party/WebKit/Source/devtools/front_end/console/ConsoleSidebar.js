// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Console.ConsoleSidebar = class extends UI.VBox {
  /**
   * @param {!ProductRegistry.BadgePool} badgePool
   */
  constructor(badgePool) {
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
    var icons = [
      UI.Icon.create('largeicon-navigator-folder'), UI.Icon.create('smallicon-clear-error'),
      UI.Icon.create('smallicon-clear-warning', 'icon-warning'), UI.Icon.create('smallicon-clear-info'),
      UI.Icon.create('smallicon-clear-warning')
    ];
    /** @type {!Array<!Console.ConsoleSidebar.FilterTreeElement>} */
    this._treeElements = [];
    for (var i = 0; i < filters.length; i++) {
      var treeElement = new Console.ConsoleSidebar.FilterTreeElement(filters[i], icons[i], badgePool);
      this._tree.appendChild(treeElement);
      this._treeElements.push(treeElement);
    }
    this._treeElements[0].expand();
    this._treeElements[0].select();
  }

  clear() {
    if (!this._enabled)
      return;
    for (var treeElement of this._treeElements)
      treeElement.clear();
  }

  resetCounters() {
    if (!this._enabled)
      return;
    for (var treeElement of this._treeElements)
      treeElement.resetCounters();
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   */
  onMessageAdded(viewMessage) {
    if (!this._enabled)
      return;
    for (var treeElement of this._treeElements)
      treeElement.onMessageAdded(viewMessage);
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
   * @param {?Element=} badge
   */
  constructor(filter, badge) {
    super(filter.name);
    this._filter = filter;
    this._countElement = this.listItemElement.createChild('span', 'count');
    var leadingIcons = [UI.Icon.create('largeicon-navigator-file')];
    if (badge)
      leadingIcons.push(badge);
    this.setLeadingIcons(leadingIcons);
  }

  resetCounters() {
    this._filter.resetCounters();
    this.updateCounters();
  }

  updateCounters() {
    var totalCount = this._filter.errorCount + this._filter.warningCount + this._filter.infoCount;
    this._countElement.textContent = totalCount > 0 ? totalCount : '';
  }
};

Console.ConsoleSidebar.FilterTreeElement = class extends UI.TreeElement {
  /**
   * @param {!Console.ConsoleFilter} filter
   * @param {!Element} icon
   * @param {!ProductRegistry.BadgePool} badgePool
   */
  constructor(filter, icon, badgePool) {
    super(filter.name, true /* expandable */);
    this._filter = filter;
    this._badgePool = badgePool;
    /** @type {!Map<?string, !Console.ConsoleSidebar.URLGroupTreeElement>} */
    this._urlTreeElements = new Map();
    this.setLeadingIcons([icon]);
  }

  clear() {
    this._urlTreeElements.clear();
    this.removeChildren();
    this._updateCounters();
  }

  resetCounters() {
    this._filter.resetCounters();
    this._updateCounters();
    for (var child of this.children())
      child.resetCounters();
  }

  _updateCounters() {
    var totalCount = this._filter.errorCount + this._filter.warningCount + this._filter.infoCount;
    var titleText = `${totalCount ? totalCount : 'No'} ${this._filter.name}`;
    if (this._filter.name !== ConsoleModel.ConsoleMessage.MessageLevel.Info &&
        this._filter.name !== ConsoleModel.ConsoleMessage.MessageLevel.Verbose)
      titleText += `${totalCount !== 1 ? 's' : ''}`;
    this.title = titleText;
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   */
  onMessageAdded(viewMessage) {
    if (!this._filter.shouldBeVisible(viewMessage))
      return;
    var url = viewMessage.consoleMessage().url || null;
    this._filter.incrementCounters(viewMessage.consoleMessage());
    this._updateCounters();

    var child = this._urlTreeElements.get(url);
    if (!child) {
      var filter = this._filter.clone();
      var parsedURL = url ? url.asParsedURL() : null;
      if (url)
        filter.name = parsedURL ? parsedURL.displayName : url;
      else
        filter.name = Common.UIString('<other>');
      filter.parsedFilters.push({key: Console.ConsoleFilter.FilterType.Url, text: url, negative: false});
      var badge = null;
      if (parsedURL)
        badge = this._badgePool.badgeForURL(parsedURL);
      child = new Console.ConsoleSidebar.URLGroupTreeElement(filter, badge);
      if (url)
        child.tooltip = url;
      this._urlTreeElements.set(url, child);
      this.appendChild(child);
    }
    child._filter.incrementCounters(viewMessage.consoleMessage());
    child.updateCounters();
  }
};
