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

    this._items.replaceAll(Console.ConsoleSidebar._createArtificialGroups());
    this._list.selectItem(this._items.at(0));

    /** @type {!Set<!Console.ConsoleSidebar.GroupItem>} */
    this._pendingItemsToAdd = new Set();
    this._pendingClear = false;

    this._keyToItemMaps = {
      'source': new Map(),
      'context': new Map(),
      'executionContext': new Map(),
      // 'url': new Map()
    };
  }

  /**
   * @return {!Array<!Console.ConsoleSidebar.GroupItem>}
   */
  static _createArtificialGroups() {
    return [
      {type: Console.ConsoleSidebar.GroupType.All, name: 'All'},
    ];
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
   * @param {*} key
   * @param {!Console.ConsoleSidebar.GroupType} type
   * @param {?ConsoleModel.ConsoleMessage.MessageLevel} level
   * @param {string} name
   * @return {?Console.ConsoleSidebar.GroupItem}
   */
  _addGroupItem(key, type, level, name) {
    var item = this._keyToItemMaps[type].get(key);
    if (!item) {
      item = {name: name, key: key, info: 0, warning: 0, error: 0, dirty: false, type: type};
      this._keyToItemMaps[type].set(key, item);
      this._pendingItemsToAdd.add(item);
    } else {
      item.dirty = true;
    }
    if (level === 'info' || level === 'warning' || level === 'error')
        item[level]++;
  }

  /**
   * @param {string} context
   * @param {?ConsoleModel.ConsoleMessage.MessageLevel} level
   */
  addMessage(message) {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;

    var level = message.level;
    var source = message.source;
    var executionContext = message.runtimeModel() && message.runtimeModel().executionContext(message.executionContextId);
    var isViolation = source === ConsoleModel.ConsoleMessage.MessageSource.Violation;
    if (isViolation)
      this._addGroupItem(source, 'source', level, source);
    if (message.context)
      this._addGroupItem(message.context, 'context', level, message.context);
    if (!isViolation) {
      if (executionContext)
        this._addGroupItem(executionContext, 'executionContext', level, executionContext.displayTitle());
      // if (message.url)
      //   this._addGroupItem(message.url, 'url', level, message.url);
    }
  }

  clear() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    for (var key in this._keyToItemMaps)
      this._keyToItemMaps[key].clear();
    this._pendingItemsToAdd.clear();
    this._pendingClear = true;
  }

  refresh() {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;
    if (this._pendingClear) {
      this._items.replaceAll(Console.ConsoleSidebar._createArtificialGroups());
      this._list.selectItem(this._items.at(0));
      this._pendingClear = false;
    }
    // Refresh counters for stale groups.
    this._list.refreshItem(this._items.at(0));
    for (var item of this._items) {
      if (item.dirty)
        this._list.refreshItem(item);
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
    element.tabIndex = 0;

    if (item.type === Console.ConsoleSidebar.GroupType.All) {
      var warnCount = ConsoleModel.consoleModel.warnings();
      var errorCount = ConsoleModel.consoleModel.errors();
      var infoCount = ConsoleModel.consoleModel.messages().length - warnCount - errorCount;
      element.appendChild(createCounters(infoCount, warnCount, errorCount));
    } else {
      element.appendChild(createCounters(item.info, item.warning, item.error));
    }

    /**
     * @param {number} info
     * @param {number} warning
     * @param {number} error
     * @return {!Element}
     */
    function createCounters(info, warning, error) {
      var counters = element.createChild('div', 'counters');
      if (error)
        counters.createChild('span', 'error-count').textContent = error > 99 ? '99+' : error;
      if (warning)
        counters.createChild('span', 'warning-count').textContent = warning > 99 ? '99+' : warning;
      if (info)
        counters.createChild('span', 'info-count').textContent = info > 99 ? '99+' : info;
      return counters;
    }
    return element;
  }

  /**
   * @override
   * @param {!Console.ConsoleSidebar.GroupItem} item
   * @return {number}
   */
  heightForItem(item) {
    return 26;
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
  All: 'All'
};

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  ContextSelected: Symbol('ContextSelected')
};

/** @typedef {{context: (string|symbol), name: string, info: number, warning, number, error: number, dirty: boolean}} */
Console.ConsoleSidebar.GroupItem;
