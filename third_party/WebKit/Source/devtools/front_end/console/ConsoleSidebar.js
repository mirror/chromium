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

    this._allGroupInstance = this._allGroup();
    this._items.replaceAll([this._allGroup()]);
    this._list.selectItem(this._items.at(0));
    this._valueToItemMaps = this._createValueToItemMaps();
    /** @type {!Set<!Console.ConsoleSidebar.GroupItem>} */
    this._pendingItemsToAdd = new Set();
    this._pendingClear = false;
  }

  /**
   * @return {!Console.ConsoleSidebar.GroupItem}
   */
  _allGroup() {
    if (this._allGroupInstance)
      return this._allGroupInstance;
    this._allGroupInstance =
        {type: Console.ConsoleSidebar.GroupType.All, name: 'All', value: 'All', info: 0, warning: 0, error: 0};
    return this._allGroupInstance;
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
    var level = message.level;
    var context = message.context;
    var isViolation = message.source === ConsoleModel.ConsoleMessage.MessageSource.Violation;
    var executionContext =
        message.runtimeModel() && message.runtimeModel().executionContext(message.executionContextId);
    this._addGroupItem(level, Console.ConsoleSidebar.GroupType.All, this._allGroup().value, this._allGroup().name);
    if (context)
      this._addGroupItem(level, Console.ConsoleSidebar.GroupType.Context, context, context);
    if (isViolation) {
      this._addGroupItem(
          level, Console.ConsoleSidebar.GroupType.Violation, message.source, Common.UIString('Violations'));
    } else if (executionContext) {
      this._addGroupItem(
          level, Console.ConsoleSidebar.GroupType.ExecutionContext, executionContext, executionContext.displayTitle());
    }
  }

  /**
   * @param {?ConsoleModel.ConsoleMessage.MessageLevel} level
   * @param {!Console.ConsoleSidebar.GroupType} type
   * @param {string|!SDK.ExecutionContext} value
   * @param {string} name
   */
  _addGroupItem(level, type, value, name) {
    var item = this._valueToItemMaps[type].get(value);
    if (!item) {
      item = {type: type, name: name, value: value, info: 0, warning: 0, error: 0};
      this._valueToItemMaps[type].set(value, item);
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
   * @return {!Object<!Console.ConsoleSidebar.GroupType, !Map<(string|!SDK.ExecutionContext), !Console.ConsoleSidebar.GroupItem>>}
   */
  _createValueToItemMaps() {
    /** @type {!Object<!Console.ConsoleSidebar.GroupType, !Map<(string|!SDK.ExecutionContext), !Console.ConsoleSidebar.GroupItem>>} */
    var valueToItemMaps = {};
    for (var type of Object.values(Console.ConsoleSidebar.GroupType)) {
      valueToItemMaps[type] =
          /** @type {!Map<(string|!SDK.ExecutionContext), !Console.ConsoleSidebar.GroupItem>} */ (new Map());
    }
    valueToItemMaps[Console.ConsoleSidebar.GroupType.All].set(this._allGroup().name, this._allGroup());
    return valueToItemMaps;
  }

  refresh() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    if (this._pendingClear) {
      this._allGroup().info = 0;
      this._allGroup().warning = 0;
      this._allGroup().error = 0;
      this._items.replaceAll([this._allGroup()]);
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
    element.createChild('div', 'name').textContent = item.name.trimMiddle(30);
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
    this.dispatchEventToListeners(Console.ConsoleSidebar.Events.ContextSelected, to);
  }
};

/** @enum {symbol} */
Console.ConsoleSidebar.GroupType = {
  All: Symbol('All'),
  Context: Symbol('Context'),
  ExecutionContext: Symbol('ExecutionContext'),
  Violation: Symbol('Violation')
};

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  ContextSelected: Symbol('ContextSelected')
};

/** @typedef {{
        type: symbol,
        name: string,
        value: (string|!SDK.ExecutionContext),
        info: number,
        warning: number,
        error: number
    }}
 */
Console.ConsoleSidebar.GroupItem;

/** @type {symbol} */
Console.ConsoleSidebar._itemIsDirtySymbol = Symbol('itemIsDirty');
