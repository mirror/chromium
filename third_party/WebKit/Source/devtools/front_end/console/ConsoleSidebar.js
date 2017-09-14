// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {UI.ListDelegate<!Console.ConsoleFilter>}
 */
Console.ConsoleSidebar = class extends UI.VBox {
  constructor() {
    super(true);
    this.registerRequiredCSS('console/consoleSidebar.css');
    this.setMinimumSize(50, 0);

    /** @type {!UI.ListModel<!Console.ConsoleFilter>} */
    this._items = new UI.ListModel();
    /** @type {!UI.ListControl<!Console.ConsoleFilter>} */
    this._list = new UI.ListControl(this._items, this, UI.ListMode.EqualHeightItems);
    this._list.element.classList.add('list');
    this.contentElement.appendChild(this._list.element);

    this._allFilter =
        new Console.ConsoleFilter(Common.UIString('All'), null, Console.ConsoleViewFilter.allLevelsFilterValue());
    var defaultParsedFilter = /** @type {!TextUtils.FilterParser.ParsedFilter} */ ({
      key: Console.ConsoleViewFilter.FilterType.Level,
      text: ConsoleModel.ConsoleMessage.MessageLevel.Verbose,
      negative: true
    });
    this._defaultFilter = new Console.ConsoleFilter(
        Common.UIString('Default'), defaultParsedFilter, Console.ConsoleViewFilter.defaultLevelsFilterValue());
    this._items.replaceAll([this._allFilter, this._defaultFilter]);
    this._list.selectItem(this._items.at(0));
    this._valueToItemMaps = this._createValueToItemMaps();
    /** @type {!Set<!Console.ConsoleFilter>} */
    this._pendingItemsToAdd = new Set();
    this._pendingClear = false;
  }

  /**
   * @override
   */
  wasShown() {
    // ListControl's viewport does not update when hidden.
    this._list.viewportResized();
  }

  /**
   * @override
   */
  onResize() {
    this._list.viewportResized();
  }

  /**
   * @param {!ConsoleModel.ConsoleMessage} message
   */
  onMessageAdded(message) {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    this._allFilter[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
    this._allFilter.incrementCounters(message.level);
    if (message.level !== ConsoleModel.ConsoleMessage.MessageLevel.Verbose) {
      this._defaultFilter[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
      this._defaultFilter.incrementCounters(message.level);
    }
    if (message.context)
      this._addItem(message.level, Console.ConsoleViewFilter.FilterType.Context, message.context);
  }

  /**
   * @param {?ConsoleModel.ConsoleMessage.MessageLevel} level
   * @param {!Console.ConsoleViewFilter.FilterType} filterType
   * @param {string} value
   */
  _addItem(level, filterType, value) {
    var item = this._valueToItemMaps[filterType].get(value);
    if (!item) {
      item = new Console.ConsoleFilter(
          value, /** @type {!TextUtils.FilterParser.ParsedFilter} */ ({key: filterType, text: value, negative: false}));
      this._valueToItemMaps[filterType].set(value, item);
      this._pendingItemsToAdd.add(item);
    } else {
      item[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
    }
    item.incrementCounters(level);
  }

  clear() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    this._valueToItemMaps = this._createValueToItemMaps();
    this._pendingItemsToAdd.clear();
    this._pendingClear = true;
  }

  /**
   * @return {!Object<!Console.ConsoleViewFilter.FilterType, !Map<string, !Console.ConsoleFilter>>}
   */
  _createValueToItemMaps() {
    /** @type {!Object<!Console.ConsoleViewFilter.FilterType, !Map<string, !Console.ConsoleFilter>>} */
    var valueToItemMaps = {};
    for (var type of Object.values(Console.ConsoleViewFilter.FilterType))
      valueToItemMaps[type] = /** @type {!Map<string, !Console.ConsoleFilter>} */ (new Map());
    return valueToItemMaps;
  }

  refresh() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    if (this._pendingClear) {
      this._allFilter.resetCounters();
      this._defaultFilter.resetCounters();
      this._items.replaceAll([this._allFilter, this._defaultFilter]);
      this._list.selectItem(this._items.at(0));
      this._pendingClear = false;
    }
    // Refresh counters for stale groups.
    for (var item of this._items) {
      if (item[Console.ConsoleSidebar._itemIsDirtySymbol]) {
        this._list.refreshItem(item);
        delete item[Console.ConsoleSidebar._itemIsDirtySymbol];
      }
    }

    // Add new groups.
    if (this._pendingItemsToAdd.size > 0) {
      this._items.replaceRange(this._items.length, this._items.length, Array.from(this._pendingItemsToAdd));
      this._pendingItemsToAdd.clear();
    }
  }

  /**
   * @override
   * @param {!Console.ConsoleFilter} item
   * @return {!Element}
   */
  createElementForItem(item) {
    var element = createElementWithClass('div', 'context-item');
    element.createChild('div', 'name').textContent = item.name();
    element.title = item.name();
    var countersElement = element.createChild('div', 'counters');
    var counts = item.counters();
    if (counts.error)
      countersElement.createChild('span', 'error-count').textContent = counts.error > 99 ? '99+' : counts.error;
    if (counts.warning)
      countersElement.createChild('span', 'warning-count').textContent = counts.warning > 99 ? '99+' : counts.warning;
    if (counts.info)
      countersElement.createChild('span', 'info-count').textContent = counts.info > 99 ? '99+' : counts.info;
    return element;
  }

  /**
   * @override
   * @param {!Console.ConsoleFilter} item
   * @return {number}
   */
  heightForItem(item) {
    return 28;
  }

  /**
   * @override
   * @param {!Console.ConsoleFilter} item
   * @return {boolean}
   */
  isItemSelectable(item) {
    return true;
  }

  /**
   * @override
   * @param {?Console.ConsoleFilter} from
   * @param {?Console.ConsoleFilter} to
   * @param {?Element} fromElement
   * @param {?Element} toElement
   */
  selectedItemChanged(from, to, fromElement, toElement) {
    if (fromElement)
      fromElement.classList.remove('selected');
    if (!to || !toElement)
      return;

    toElement.classList.add('selected');
    this.dispatchEventToListeners(Console.ConsoleSidebar.Events.GroupSelected, to);
  }
};

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  GroupSelected: Symbol('GroupSelected')
};

/** @type {symbol} */
Console.ConsoleSidebar._itemIsDirtySymbol = Symbol('itemIsDirty');

Console.ConsoleFilter = class {
  /**
   * @param  {string} name
   * @param  {?TextUtils.FilterParser.ParsedFilter} parsedFilter
   * @param  {!Object<string, boolean>=} levelsMask
   */
  constructor(name, parsedFilter, levelsMask) {
    this._name = name;
    this._parsedFilter = parsedFilter;
    this._levelsMask = levelsMask || Console.ConsoleViewFilter.defaultLevelsFilterValue();

    this._info = 0;
    this._warning = 0;
    this._error = 0;
  }

  /**
   * @return {string}
   */
  name() {
    return this._name;
  }

  /**
   * @return {?TextUtils.FilterParser.ParsedFilter}
   */
  parsedFilter() {
    return this._parsedFilter;
  }

  /**
   * @return {!Object<string, number>}
   */
  counters() {
    return {info: this._info, warning: this._warning, error: this._error};
  }

  /**
   * @param {?ConsoleModel.ConsoleMessage.MessageLevel} level
   */
  incrementCounters(level) {
    if (level === ConsoleModel.ConsoleMessage.MessageLevel.Info ||
        level === ConsoleModel.ConsoleMessage.MessageLevel.Verbose)
      this._info++;
    else if (level === ConsoleModel.ConsoleMessage.MessageLevel.Warning)
      this._warning++;
    else if (level === ConsoleModel.ConsoleMessage.MessageLevel.Error)
      this._error++;
  }

  resetCounters() {
    this._info = 0;
    this._warning = 0;
    this._error = 0;
  }
};
