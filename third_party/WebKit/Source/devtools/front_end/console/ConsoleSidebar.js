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
    this._filters = new UI.ListModel();
    /** @type {!UI.ListControl<!Console.ConsoleFilter>} */
    this._list = new UI.ListControl(this._filters, this, UI.ListMode.EqualHeightItems);
    this._list.element.classList.add('list');
    this.contentElement.appendChild(this._list.element);

    this._allFilter =
        new Console.ConsoleFilter(Common.UIString('All'), [], Console.ConsoleFilter.allLevelsFilterValue());
    this._filters.replaceAll([this._allFilter]);
    this._list.selectItem(this._filters.at(0));

    /** @type {!Map<string, !Console.ConsoleFilter>} */
    this._contextFilters = new Map();
    /** @type {!Map<!SDK.ExecutionContext, !Console.ConsoleFilter>} */
    this._executionContextFilters = new Map();
    /** @type {!Set<!Console.ConsoleFilter>} */
    this._pendingFiltersToAdd = new Set();
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

    var context = message.context;
    if (context) {
      var item = this._contextFilters.get(context);
      if (!item) {
        var filter = {key: Console.ConsoleFilter.FilterType.Context, text: context, negative: false};
        item = new Console.ConsoleFilter(context, [filter]);
        this._contextFilters.set(context, item);
        this._pendingFiltersToAdd.add(item);
      } else {
        item[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
      }
    }

    var runtimeModel = message.runtimeModel();
    var executionContext = runtimeModel && runtimeModel.executionContext(message.executionContextId);
    if (executionContext) {
      var item = this._executionContextFilters.get(executionContext);
      if (!item) {
        item = new Console.ConsoleFilter(
            executionContext.displayTitle(), [], Console.ConsoleFilter.allLevelsFilterValue(), executionContext);
        this._executionContextFilters.set(executionContext, item);
        this._pendingFiltersToAdd.add(item);
      } else {
        item[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
      }
    }
  }

  clear() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    this._contextFilters.clear();
    this._executionContextFilters.clear();
    this._pendingFiltersToAdd.clear();
    this._pendingClear = true;
  }

  /**
   * @return {boolean}
   */
  applyFilters(viewMessage) {
    var passed = true;
    for (var item of this._filters) {
      var passedFilter = item.applyFilter(viewMessage);
      if (item === this._list.selectedItem())
        passed = passedFilter;
    }
    for (var item of this._pendingFiltersToAdd)
      item.applyFilter(viewMessage);
    return passed;
  }

  resetItemCounters() {
    for (var item of this._filters) {
      item.resetCounters();
      item[Console.ConsoleSidebar._itemIsDirtySymbol] = true;
    }
  }

  refresh() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    if (this._pendingClear) {
      this._filters.replaceAll([this._allFilter]);
      this._list.selectItem(this._filters.at(0));
      this._pendingClear = false;
    }
    // Refresh counters for stale groups.
    for (var item of this._filters) {
      if (item[Console.ConsoleSidebar._itemIsDirtySymbol]) {
        this._list.refreshItem(item);
        delete item[Console.ConsoleSidebar._itemIsDirtySymbol];
      }
    }

    // Add new groups.
    if (this._pendingFiltersToAdd.size > 0) {
      this._filters.replaceRange(this._filters.length, this._filters.length, Array.from(this._pendingFiltersToAdd));
      this._pendingFiltersToAdd.clear();
    }
  }

  /**
   * @override
   * @param {!Console.ConsoleFilter} item
   * @return {!Element}
   */
  createElementForItem(item) {
    var element = createElementWithClass('div', 'context-item');
    element.createChild('div', 'name').textContent = item.name.trimMiddle(30);
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
    this.dispatchEventToListeners(Console.ConsoleSidebar.Events.FilterSelected, to);
  }
};

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  FilterSelected: Symbol('FilterSelected')
};

/** @type {symbol} */
Console.ConsoleSidebar._itemIsDirtySymbol = Symbol('itemIsDirty');
