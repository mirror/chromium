// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {SDK.TargetManager.Observer}
 * @implements {UI.ListDelegate<!Protocol.Target.TargetInfo>}
 */
Discovery.DiscoveryView = class extends UI.VBox {
  /**
   * @param {function()} switchToTargetCallback
   */
  constructor(switchToTargetCallback) {
    super(true);
    this.registerRequiredCSS('discovery/discoveryView.css');
    this._switchToTargetCallback = switchToTargetCallback;

    /** @type {!UI.ListModel<!Protocol.Target.TargetInfo>} */
    this._items = new UI.ListModel();
    /** @type {!UI.ListControl<!Protocol.Target.TargetInfo>} */
    this._list = new UI.ListControl(this._items, this, UI.ListMode.EqualHeightItems);
    this._list.element.classList.add('discovery-list');
    this.contentElement.appendChild(this._list.element);

    SDK.targetManager.addEventListener(SDK.TargetManager.Events.AvailableTargetAdded, this._availableTargetAdded, this);
    SDK.targetManager.addEventListener(
        SDK.TargetManager.Events.AvailableTargetChanged, this._availableTargetChanged, this);
    SDK.targetManager.addEventListener(
        SDK.TargetManager.Events.AvailableTargetRemoved, this._availableTargetRemoved, this);
    SDK.targetManager.observeTargets(this);
  }

  /**
   * @param {!Common.Event} event
   */
  _availableTargetAdded(event) {
    var targetInfo = /** @type {!Protocol.Target.TargetInfo} */ (event.data);
    if (targetInfo.type === 'browser')
      return;
    this._items.insert(this._items.length, targetInfo);
  }

  /**
   * @param {!Common.Event} event
   */
  _availableTargetChanged(event) {
    var targetInfo = /** @type {!Protocol.Target.TargetInfo} */ (event.data);
    var index = this._items.findIndex(info => info.targetId === targetInfo.targetId);
    if (index !== -1)
      this._items.replace(index, targetInfo);
  }

  /**
   * @param {!Common.Event} event
   */
  _availableTargetRemoved(event) {
    var targetInfo = /** @type {!Protocol.Target.TargetInfo} */ (event.data);
    var index = this._items.findIndex(info => info.targetId === targetInfo.targetId);
    if (index !== -1)
      this._items.remove(index);
  }

  /**
   * @override
   * @param {!SDK.Target} target
   */
  targetAdded(target) {
    var targetInfo = this._items.find(info => info.targetId === target.id());
    if (targetInfo)
      this._list.refreshItem(targetInfo);
  }

  /**
   * @override
   * @param {!SDK.Target} target
   */
  targetRemoved(target) {
    var targetInfo = this._items.find(info => info.targetId === target.id());
    if (targetInfo)
      this._list.refreshItem(targetInfo);
  }

  /**
   * @override
   */
  onResize() {
    this._list.viewportResized();
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {!Element}
   */
  createElementForItem(targetInfo) {
    var element = createElementWithClass('div', 'discovery-item');
    element.createChild('div', 'discovery-title').textContent = targetInfo.title;
    element.createChild('div', 'discovery-type').textContent = targetInfo.type;
    var isAttached = !!SDK.targetManager.targetById(targetInfo.targetId);
    element.createChild('div', 'discovery-attached').textContent =
        isAttached ? 'we are attached' : (targetInfo.attached ? 'someone attached' : 'detached');
    element.createChild('div', 'discovery-url').textContent = targetInfo.url;
    element.addEventListener('click', () => {
      if (!isAttached)
        SDK.targetManager.attachToAvailableTarget(targetInfo);
      this._switchToTargetCallback.call(null);
    }, false);
    return element;
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {number}
   */
  heightForItem(targetInfo) {
    return 0;  // Let list measure for us.
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {boolean}
   */
  isItemSelectable(targetInfo) {
    return false;
  }

  /**
   * @override
   * @param {?Protocol.Target.TargetInfo} from
   * @param {?Protocol.Target.TargetInfo} to
   * @param {?Element} fromElement
   * @param {?Element} toElement
   */
  selectedItemChanged(from, to, fromElement, toElement) {
  }
};
