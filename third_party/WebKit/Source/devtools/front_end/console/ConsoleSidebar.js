// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {UI.ListDelegate<!Console.ConsoleSidebar.GroupItem>}
 */
Console.ConsoleSidebar = class extends UI.VBox {
  constructor() {
    super(true);
    this.registerRequiredCSS('console/consoleSidebar.css');
    this.setMinimumSize(50, 0);

    /** @type {!UI.ListModel<!Console.ConsoleSidebar.GroupItem>} */
    this._items = new UI.ListModel();
    /** @type {!UI.ListControl<!Console.ConsoleSidebar.GroupItem>} */
    this._list = new UI.ListControl(this._items, this, UI.ListMode.EqualHeightItems);
    this._list.element.classList.add('list');
    this.contentElement.appendChild(this._list.element);

    this._allGroup = /** @type {!Console.ConsoleSidebar.GroupItem} */ ({
      type: Console.ConsoleSidebar.AllGroupType,
      name: 'All',
      value: 'All',
      info: 0,
      warning: 0,
      error: 0,
      filter: null
    });
    this._items.replaceAll([this._allGroup]);
    this._list.selectItem(this._items.at(0));
    this._valueToItemMaps = this._createValueToItemMaps();
    /** @type {!Set<!Console.ConsoleSidebar.GroupItem>} */
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
    this._addItem(message.level, Console.ConsoleSidebar.AllGroupType, this._allGroup.name);
    if (message.context)
      this._addItem(message.level, Console.ConsoleViewFilter.FilterType.Context, message.context);
    if (message.source === ConsoleModel.ConsoleMessage.MessageSource.Violation)
      this._addItem(message.level, Console.ConsoleViewFilter.FilterType.Source, message.source);
  }

  /**
   * @param {?ConsoleModel.ConsoleMessage.MessageLevel} level
   * @param {!Console.ConsoleSidebar.GroupType} key
   * @param {string} value
   */
  _addItem(level, key, value) {
    var item = key === Console.ConsoleSidebar.AllGroupType ? this._allGroup : this._valueToItemMaps[key].get(value);
    if (!item) {
      item = /** @type {!Console.ConsoleSidebar.GroupItem} */ (
          {name: value, info: 0, warning: 0, error: 0, filter: {key: key, text: value, negative: false}});
      this._valueToItemMaps[key].set(value, item);
      this._pendingItemsToAdd.add(item);
    } else {
      item[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
    }
    if (level === ConsoleModel.ConsoleMessage.MessageLevel.Info ||
        level === ConsoleModel.ConsoleMessage.MessageLevel.Verbose)
      item.info++;
    else if (level === ConsoleModel.ConsoleMessage.MessageLevel.Warning)
      item.warning++;
    else if (level === ConsoleModel.ConsoleMessage.MessageLevel.Error)
      item.error++;
  }

  clear() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    this._valueToItemMaps = this._createValueToItemMaps();
    this._pendingItemsToAdd.clear();
    this._pendingClear = true;
  }

  /**
   * @return {!Object<!Console.ConsoleSidebar.GroupType, !Map<string, !Console.ConsoleSidebar.GroupItem>>}
   */
  _createValueToItemMaps() {
    /** @type {!Object<!Console.ConsoleSidebar.GroupType, !Map<string, !Console.ConsoleSidebar.GroupItem>>} */
    var valueToItemMaps = {};
    valueToItemMaps[Console.ConsoleSidebar.AllGroupType] = new Map([[this._allGroup.name, this._allGroup]]);
    for (var type of Object.values(Console.ConsoleViewFilter.FilterType))
      valueToItemMaps[type] = /** @type {!Map<string, !Console.ConsoleSidebar.GroupItem>} */ (new Map());
    return valueToItemMaps;
  }

  refresh() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    if (this._pendingClear) {
      this._allGroup.info = 0;
      this._allGroup.warning = 0;
      this._allGroup.error = 0;
      this._items.replaceAll([this._allGroup]);
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
   * @param {!Console.ConsoleSidebar.GroupItem} item
   * @return {!Element}
   */
  createElementForItem(item) {
    var element = createElementWithClass('div', 'context-item');
    element.createChild('div', 'name').textContent = item.name;
    element.title = item.name;
    var counters = element.createChild('div', 'counters');
    if (item.error)
      counters.createChild('span', 'error-count').textContent = item.error > 99 ? '99+' : item.error;
    if (item.warning)
      counters.createChild('span', 'warning-count').textContent = item.warning > 99 ? '99+' : item.warning;
    if (item.info)
      counters.createChild('span', 'info-count').textContent = item.info > 99 ? '99+' : item.info;
    return element;
  }

  /**
   * @override
   * @param {!Console.ConsoleSidebar.GroupItem} item
   * @return {number}
   */
  heightForItem(item) {
    return 28;
  }

  /**
   * @override
   * @param {!Console.ConsoleSidebar.GroupItem} item
   * @return {boolean}
   */
  isItemSelectable(item) {
    return true;
  }

  /**
   * @override
   * @param {?Console.ConsoleSidebar.GroupItem} from
   * @param {?Console.ConsoleSidebar.GroupItem} to
   * @param {?Element} fromElement
   * @param {?Element} toElement
   */
  selectedItemChanged(from, to, fromElement, toElement) {
    if (fromElement)
      fromElement.classList.remove('selected');
    if (!to || !toElement)
      return;

    toElement.classList.add('selected');
    this.dispatchEventToListeners(Console.ConsoleSidebar.Events.GroupSelected, to.filter);
  }
};

Console.ConsoleSidebar.AllGroupType = Symbol('All');

/** @typedef {!Console.ConsoleViewFilter.FilterType|symbol} */
Console.ConsoleSidebar.GroupType;

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  GroupSelected: Symbol('GroupSelected')
};

/** @typedef {{
        name: string,
        value: string,
        filter: ?TextUtils.FilterParser.ParsedFilter,
        info: number,
        warning: number,
        error: number
    }}
 */
Console.ConsoleSidebar.GroupItem;

/** @type {symbol} */
Console.ConsoleSidebar._itemIsDirtySymbol = Symbol('itemIsDirty');
