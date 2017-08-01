// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements UI.ListDelegate
 */
Console.ConsoleSidebar = class extends UI.VBox {
  constructor() {
    super(true);
    this.registerRequiredCSS('console/consoleSidebar.css');
    this.contentElement.classList.add('content');
    this.setMinimumSize(50, 0);

    this._items = new UI.ListModel();
    this._list = new UI.ListControl(this._items, this, UI.ListMode.EqualHeightItems);
    this.contentElement.appendChild(this._list.element);

    this._items.replaceAll([Console.ConsoleSidebar._createAllGroup()]);
    this._list.selectItem(this._items.at(0));
  }

  /**
   * @return {!Console.ConsoleSidebar.GroupItem}
   */
  static _createAllGroup() {
    return {context: Console.Contexts.All, name: 'All'};
  }

  /**
   * @param {!Array<!Console.ConsoleSidebar.GroupItem>} items
   */
  updateGroupItems(items) {
    var lastSelectedContext = this._list.selectedItem() ? this._list.selectedItem().context : null;
    var newItems = [Console.ConsoleSidebar._createAllGroup()].concat(items);
    this._items.replaceAll(newItems);
    if (lastSelectedContext) {
      var item = this._items.find(item => item.context === lastSelectedContext);
      if (item)
        this._list.selectItem(item);
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
    if (!to || !toElement)
      return;
    if (fromElement)
      fromElement.classList.remove('selected');

    toElement.classList.add('selected');
    this.dispatchEventToListeners(Console.ConsoleSidebar.Events.ContextSelected, to.context);
  }
};

Console.Contexts = {
  All: Symbol('All')
};

Console.ConsoleSidebar.Events = {
  ContextSelected: Symbol('ContextSelected')
};

/** @typedef {{context: (string|symbol), name: string}} */
Console.ConsoleSidebar.GroupItem;
