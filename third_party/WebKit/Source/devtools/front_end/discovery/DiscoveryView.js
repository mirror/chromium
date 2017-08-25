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

    /** @type {?string} */
    this._attachedTargetId = null;
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
    if (target.id() === this._attachedTargetId)
      this._attachedTargetId = null;
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

  _detachFromCurrent() {
    var attachedTarget = this._attachedTargetId ? SDK.targetManager.targetById(this._attachedTargetId) : null;
    if (attachedTarget)
      SDK.targetManager.detachFromTarget(attachedTarget);
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {!Element}
   */
  createElementForItem(targetInfo) {
    var className = Discovery.DiscoveryView._typeToClass[targetInfo.type] || 'discovery-type-other';
    var typeText = Discovery.DiscoveryView._typeToText[targetInfo.type] || Common.UIString('other');
    var isAttached = !!SDK.targetManager.targetById(targetInfo.targetId);
    var title = targetInfo.title || Common.UIString('<untitled>');
    var canAttach = targetInfo.type === 'page';

    var element = createElementWithClass('div', 'discovery-item');
    var info = element.createChild('div', 'discovery-info');
    var actions = element.createChild('div', 'discovery-actions');

    var titleElement = info.createChild('div', 'discovery-title');
    titleElement.createChild('div', 'discovery-type ' + className).textContent = typeText;
    var titleSpan = titleElement.createChild('span');
    titleSpan.textContent = title;
    titleSpan.title = title;
    info.createChild('div', 'discovery-url').appendChild(UI.createExternalLink(targetInfo.url));

    if (isAttached) {
      actions.appendChild(UI.createTextButton(
          Common.UIString('Inspect \u003E'), () => this._switchToTargetCallback.call(null), 'discovery-button', true));
      if (this._attachedTargetId === targetInfo.targetId) {
        actions.appendChild(UI.createTextButton(
            Common.UIString('Disconnect'), () => this._detachFromCurrent(), 'discovery-button', true));
      }
    } else if (canAttach) {
      actions.appendChild(UI.createTextButton(Common.UIString('Inspect \u003E'), () => {
        this._detachFromCurrent();
        SDK.targetManager.attachToAvailableTarget(targetInfo);
        this._attachedTargetId = targetInfo.targetId;
        this._switchToTargetCallback.call(null);
      }, 'discovery-button discovery-button-hover', true));
    } else {
      actions.appendChild(UI.createTextButton(Common.UIString('Inspect \u29c9'), () => {
        SDK.targetManager.inspectAvailableTarget(targetInfo);
      }, 'discovery-button discovery-button-hover', true));
    }

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

Discovery.DiscoveryView._typeToClass = {
  'page': 'discovery-type-page',
  'iframe': 'discovery-type-iframe',
  'background_page': 'discovery-type-extension',
  'app': 'discovery-type-app',
  'node': 'discovery-type-node',
  'service_worker': 'discovery-type-service-worker',
};

Discovery.DiscoveryView._typeToText = {
  'page': Common.UIString('page'),
  'iframe': Common.UIString('iframe'),
  'background_page': Common.UIString('extension'),
  'app': Common.UIString('app'),
  'node': Common.UIString('node'),
  'service_worker': Common.UIString('service worker'),
};
