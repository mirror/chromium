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
        new Console.ConsoleFilter(Common.UIString('All'), [], Console.ConsoleViewFilter.allLevelsFilterValue());
    this._items.replaceAll([this._allFilter]);
    this._list.selectItem(this._items.at(0));

    /** @type {!Map<string, !Console.ConsoleFilter>} */
    this._contextToItem = new Map();
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
    var context = message.context;
    if (context) {
      var item = this._contextToItem.get(context);
      if (!item) {
        var filter = {key: Console.ConsoleFilter.FilterType.Context, text: context, negative: false};
        item = new Console.ConsoleFilter(context, [filter]);
        this._contextToItem.set(context, item);
        this._pendingItemsToAdd.add(item);
      } else {
        item[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
      }
      item.incrementCounters(message.level);
    }
  }

  clear() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    this._contextToItem.clear();
    this._pendingItemsToAdd.clear();
    this._pendingClear = true;
  }

  refresh() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    if (this._pendingClear) {
      this._allFilter.resetCounters();
      this._items.replaceAll([this._allFilter]);
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
    element.createChild('div', 'name').textContent = item.name;
    element.title = item.name;
    var counters = element.createChild('div', 'counters');
    if (item.errorCount)
      counters.createChild('span', 'error-count').textContent = item.errorCount > 99 ? '99+' : item.errorCount;
    if (item.warningCount)
      counters.createChild('span', 'warning-count').textContent = item.warningCount > 99 ? '99+' : item.warningCount;
    if (item.infoCount)
      counters.createChild('span', 'info-count').textContent = item.infoCount > 99 ? '99+' : item.infoCount;
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
