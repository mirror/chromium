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

    /** @type {!Map<string, !Console.ConsoleSidebar.GroupItem>} */
    this._contextToItem = new Map();
    /** @type {!Map<string, !Console.ConsoleSidebar.GroupItem>} */
    this._sourceToItem = new Map();
    /** @type {!Map<string, !Console.ConsoleSidebar.GroupItem>} */
    this._urlToItem = new Map();
    /** @type {!Map<string, !Console.ConsoleSidebar.GroupItem>} */
    this._contextToItem = new Map();
    /** @type {!Set<!Console.ConsoleSidebar.GroupItem>} */
    this._pendingItemsToAdd = new Set();
    this._pendingClear = false;
  }

  /**
   * @return {!Array<!Console.ConsoleSidebar.GroupItem>}
   */
  static _createArtificialGroups() {
    return [
      {type: Console.ConsoleSidebar.GroupType.All, name: 'All'},
      {type: Console.ConsoleSidebar.GroupType.Violations, name: 'Violations'}
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
   * @param {string} context
   * @param {?ConsoleModel.ConsoleMessage.MessageLevel} level
   */
  addMessage(message) {
    if (!Runtime.experiments.isEnabled('logManagement'))
      return;

    var context = message.context;
    var level = message.level;
    var source = message.source;
    // var workerName = message.workerName();
    // var sourceGroup = Console.ConsoleSidebar._sourceToGroupId.get(source);
    // var item = this._contextToItem.get(sourceGroup.id);
    // if (!item) {
    //   item = {name: sourceGroup.name, context: sourceGroup.id, info: 0, warning: 0, error: 0, dirty: false, type: 'source'};
    //   this._contextToItem.set(sourceGroup.id, item);
    //   this._pendingItemsToAdd.add(item);
    // } else {
    //   item.dirty = true;
    // }
    // if (level === 'info' || level === 'warning' || level === 'error')
    //   item[level]++;

    if (context) {
      var item = this._contextToItem.get(context);
      if (!item) {
        item = {name: context, context: context, info: 0, warning: 0, error: 0, dirty: false, type: 'context'};
        this._contextToItem.set(context, item);
        this._pendingItemsToAdd.add(item);
      } else {
        item.dirty = true;
      }
      if (level === 'info' || level === 'warning' || level === 'error')
        item[level]++;
    }

    // // Worker source messages with a workerId must take precedence over the executionContext
    // if (message.source === ConsoleModel.ConsoleMessage.MessageSource.Worker && SDK.targetManager.targetById(message.workerId)) {
    //   var item = this._contextToItem.get(message.workerId);
    //   if (!item) {
    //     var contextName = message.workerId;
    //     item = {name: message.url, context: message.workerId, info: 0, warning: 0, error: 0, dirty: false, type: 'workerId'};
    //     this._contextToItem.set(message.workerId, item);
    //     this._pendingItemsToAdd.add(item);
    //   } else {
    //     item.dirty = true;
    //   }
    //   if (level === 'info' || level === 'warning' || level === 'error')
    //     item[level]++;
    // }

    var executionContext = message._runtimeModel && message._runtimeModel.executionContext(message.executionContextId);
    if (executionContext) {
      var item = this._contextToItem.get(executionContext);
      if (!item) {
        var contextName = executionContext.displayTitle();
        item = {name: contextName, context: executionContext, info: 0, warning: 0, error: 0, dirty: false, type: 'executionContext'};
        this._contextToItem.set(executionContext, item);
        this._pendingItemsToAdd.add(item);
      } else {
        item.dirty = true;
      }
      if (level === 'info' || level === 'warning' || level === 'error')
        item[level]++;
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
    } else if (item.type === Console.ConsoleSidebar.GroupType.Violations) {
      element.appendChild(createCounters(0, ConsoleModel.consoleModel.Violations(), 0));
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
  All: Symbol('All'),
  Violations: Symbol('Violations')
};

/** @enum {symbol} */
Console.ConsoleSidebar.Events = {
  ContextSelected: Symbol('ContextSelected')
};

/** @typedef {{context: (string|symbol), name: string, info: number, warning, number, error: number, dirty: boolean}} */
Console.ConsoleSidebar.GroupItem;

/** @type {!Map<!ConsoleModel.ConsoleMessage.MessageSource, symbol>} [description] */
Console.ConsoleSidebar._sourceToGroupId = new Map([
  [ConsoleModel.ConsoleMessage.MessageSource.XML, {id: Symbol('xml'), name: 'xml'}],
  [ConsoleModel.ConsoleMessage.MessageSource.JS, {id: Symbol('javascript'), name: 'javascript'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Network, {id: Symbol('network'), name: 'network'}],
  [ConsoleModel.ConsoleMessage.MessageSource.ConsoleAPI, {id: Symbol('console-api'), name: 'console-api'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Storage, {id: Symbol('storage'), name: 'storage'}],
  [ConsoleModel.ConsoleMessage.MessageSource.AppCache, {id: Symbol('appcache'), name: 'appcache'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Rendering, {id: Symbol('rendering'), name: 'rendering'}],
  [ConsoleModel.ConsoleMessage.MessageSource.CSS, {id: Symbol('css'), name: 'css'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Security, {id: Symbol('security'), name: 'security'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Deprecation, {id: Symbol('deprecation'), name: 'deprecation'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Worker, {id: Symbol('worker'), name: 'worker'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Violation, {id: Symbol('violation'), name: 'violation'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Intervention, {id: Symbol('intervention'), name: 'intervention'}],
  [ConsoleModel.ConsoleMessage.MessageSource.Other, {id: Symbol('other'), name: 'other'}],
]);
