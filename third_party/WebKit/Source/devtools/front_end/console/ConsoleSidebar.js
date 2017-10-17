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
    /** @type {!Array<!Console.ConsoleSidebar.FilterTreeElement>} */
    this._treeElements = [];

    var Levels = ConsoleModel.ConsoleMessage.MessageLevel;
    this._appendGroup(
        Common.UIString('message'), Console.ConsoleFilter.allLevelsFilterValue(),
        UI.Icon.create('largeicon-navigator-folder'), badgePool);
    this._appendGroup(
        Common.UIString('error'), Console.ConsoleFilter.singleLevelMask(Levels.Error),
        UI.Icon.create('smallicon-clear-error'), badgePool);
    this._appendGroup(
        Common.UIString('warning'), Console.ConsoleFilter.singleLevelMask(Levels.Warning),
        UI.Icon.create('smallicon-clear-warning', 'icon-warning'), badgePool);
    this._appendGroup(
        Console.ConsoleSidebar._irregularPluralNames.Info, Console.ConsoleFilter.singleLevelMask(Levels.Info),
        UI.Icon.create('smallicon-clear-info'), badgePool);
    this._appendGroup(
        Console.ConsoleSidebar._irregularPluralNames.Verbose, Console.ConsoleFilter.singleLevelMask(Levels.Verbose),
        UI.Icon.create('smallicon-clear-warning'), badgePool);
    this._treeElements[0].expand();
    this._treeElements[0].select();
  }

  /**
   * @param  {string} name
   * @param  {!Object<string, boolean>} levelsMask
   * @param  {!Element} icon
   * @param  {!ProductRegistry.BadgePool} badgePool
   */
  _appendGroup(name, levelsMask, icon, badgePool) {
    var filter = new Console.ConsoleFilter(name, [], null, levelsMask);
    var treeElement = new Console.ConsoleSidebar.FilterTreeElement(filter, icon, badgePool);
    this._tree.appendChild(treeElement);
    this._treeElements.push(treeElement);
  }

  clear() {
    if (!this._enabled)
      return;
    for (var treeElement of this._treeElements)
      treeElement.clear();
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
   * @param {?Element} badge
   */
  constructor(filter, badge) {
    super(filter.name);
    this._filter = filter;
    this._countElement = this.listItemElement.createChild('span', 'count');
    var leadingIcons = [UI.Icon.create('largeicon-navigator-file')];
    if (badge)
      leadingIcons.push(badge);
    this.setLeadingIcons(leadingIcons);
    this._messageCount = 0;
  }

  incrementAndUpdateCounter() {
    this._messageCount++;
    this._countElement.textContent = this._messageCount;
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
    this._messageCount = 0;
    this._updateCounter();
  }

  clear() {
    this._urlTreeElements.clear();
    this.removeChildren();
    this._messageCount = 0;
    this._updateCounter();
  }

  _updateCounter() {
    var titleText = `${this._messageCount ? this._messageCount : 'No'} ${this._filter.name}`;
    if (this._messageCount !== 1 && !Console.ConsoleSidebar._irregularPluralNameSet.has(this._filter.name))
      titleText += 's';
    this.title = titleText;
  }

  /**
   * @param {!Console.ConsoleViewMessage} viewMessage
   */
  onMessageAdded(viewMessage) {
    if (!this._filter.shouldBeVisible(viewMessage))
      return;
    var message = viewMessage.consoleMessage();
    var child = this._childElement(message.url);
    var shouldIncrementCounter = message.type !== ConsoleModel.ConsoleMessage.MessageType.Command &&
        message.type !== ConsoleModel.ConsoleMessage.MessageType.Result && !message.isGroupMessage();
    if (shouldIncrementCounter) {
      this._messageCount++;
      this._updateCounter();
      child.incrementAndUpdateCounter();
    }
  }

  /**
   * @param {string=} url
   * @return {!Console.ConsoleSidebar.URLGroupTreeElement}
   */
  _childElement(url) {
    var urlValue = url || null;
    var child = this._urlTreeElements.get(urlValue);
    if (child)
      return child;

    var filter = this._filter.clone();
    var parsedURL = urlValue ? urlValue.asParsedURL() : null;
    if (urlValue)
      filter.name = parsedURL ? parsedURL.displayName : urlValue;
    else
      filter.name = Common.UIString('<other>');
    filter.parsedFilters.push({key: Console.ConsoleFilter.FilterType.Url, text: urlValue, negative: false});
    var badge = parsedURL ? this._badgePool.badgeForURL(parsedURL) : null;
    child = new Console.ConsoleSidebar.URLGroupTreeElement(filter, badge);
    if (urlValue)
      child.tooltip = urlValue;
    this._urlTreeElements.set(urlValue, child);
    this.appendChild(child);
    return child;
  }
};

/** @enum {string} */
Console.ConsoleSidebar._irregularPluralNames = {
  Info: Common.UIString('info'),
  Verbose: Common.UIString('verbose')
};

/** @const {!Set<string>} */
Console.ConsoleSidebar._irregularPluralNameSet = new Set(Object.values(Console.ConsoleSidebar._irregularPluralNames));
