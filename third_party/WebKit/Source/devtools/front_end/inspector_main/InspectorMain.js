// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {Common.Runnable}
 */
InspectorMain.InspectorMain = class {
  constructor() {
    /** @type {!Protocol.InspectorBackend.Connection} */
    this._mainConnection;
    /** @type {function()} */
    this._webSocketConnectionLostCallback;
  }

  /**
   * @override
   */
  run() {
    SDK.targetManager.setChildTargetManagerFactory(this._createChildTargetManager.bind(this));
    this._connectAndCreateMainTarget();
    InspectorFrontendHost.connectionReady();

    new InspectorMain.InspectedNodeRevealer();
    new InspectorMain.NetworkPanelIndicator();
    new InspectorMain.SourcesPanelIndicator();
    new InspectorMain.BackendSettingsSync();
  }

  /**
   * @param {!SDK.Target} target
   * @return {?SDK.ChildTargetManagerBase}
   */
  _createChildTargetManager(target) {
    if (Runtime.queryParam('nodeFrontend'))
      return new InspectorMain.NodeChildTargetManager(target);
    return new InspectorMain.BrowserChildTargetManager(target);
  }

  _connectAndCreateMainTarget() {
    if (Runtime.queryParam('nodeFrontend'))
      Host.userMetrics.actionTaken(Host.UserMetrics.Action.ConnectToNodeJSFromFrontend);
    else if (Runtime.queryParam('v8only'))
      Host.userMetrics.actionTaken(Host.UserMetrics.Action.ConnectToNodeJSDirectly);

    var target = SDK.targetManager.createTarget(
        'main', Common.UIString('Main'), this._capabilitiesForMainTarget(), this._createMainConnection.bind(this),
        null);

    if (Runtime.queryParam('nodeFrontend'))
      target.setInspectedURL('Node.js');
    else if (target.hasTargetCapability())
      target.targetAgent().setRemoteLocations([{host: 'localhost', port: 9229}]);

    if (target.hasJSCapability())
      target.runtimeAgent().runIfWaitingForDebugger();
  }

  /**
   * @return {number}
   */
  _capabilitiesForMainTarget() {
    if (Runtime.queryParam('nodeFrontend'))
      return SDK.Target.Capability.Target;

    if (Runtime.queryParam('isSharedWorker')) {
      return SDK.Target.Capability.Browser | SDK.Target.Capability.Inspector | SDK.Target.Capability.Log |
          SDK.Target.Capability.Network | SDK.Target.Capability.Target;
    }

    if (Runtime.queryParam('v8only'))
      return SDK.Target.Capability.JS;

    return SDK.Target.Capability.Browser | SDK.Target.Capability.DOM | SDK.Target.Capability.DeviceEmulation |
        SDK.Target.Capability.Emulation | SDK.Target.Capability.Input | SDK.Target.Capability.Inspector |
        SDK.Target.Capability.JS | SDK.Target.Capability.Log | SDK.Target.Capability.Network |
        SDK.Target.Capability.ScreenCapture | SDK.Target.Capability.Security | SDK.Target.Capability.Target |
        SDK.Target.Capability.Tracing;
  }

  /**
   * @param {!Protocol.InspectorBackend.Connection.Params} params
   * @return {!Protocol.InspectorBackend.Connection}
   */
  _createMainConnection(params) {
    var wsParam = Runtime.queryParam('ws');
    var wssParam = Runtime.queryParam('wss');
    if (wsParam || wssParam) {
      var ws = wsParam ? `ws://${wsParam}` : `wss://${wssParam}`;
      this._mainConnection = new SDK.WebSocketConnection(ws, this._webSocketConnectionLostCallback, params);
    } else if (InspectorFrontendHost.isHostedMode()) {
      this._mainConnection = new SDK.StubConnection(params);
    } else {
      this._mainConnection = new SDK.MainConnection(params);
    }
    return this._mainConnection;
  }

  /**
   * @param {function(string)} onMessage
   * @return {!Promise<!Protocol.InspectorBackend.Connection>}
   */
  _interceptMainConnection(onMessage) {
    var params = {onMessage: onMessage, onDisconnect: this._connectAndCreateMainTarget.bind(this)};
    return this._mainConnection.disconnect().then(this._createMainConnection.bind(this, params));
  }

  _webSocketConnectionLost() {
    if (!InspectorMain._disconnectedScreenWithReasonWasShown)
      InspectorMain.RemoteDebuggingTerminatedScreen.show('WebSocket disconnected');
  }
};

/**
 * @param {function(string)} onMessage
 * @return {!Promise<!Protocol.InspectorBackend.Connection>}
 */
InspectorMain.interceptMainConnection = function(onMessage) {
  return self.runtime.sharedInstance(InspectorMain.InspectorMain)._interceptMainConnection(onMessage);
};

/**
 * @implements {Common.Runnable}
 */
InspectorMain.InspectorMainLate = class {
  /**
   * @override
   */
  run() {
    InspectorFrontendHost.events.addEventListener(
        InspectorFrontendHostAPI.Events.ReloadInspectedPage, this._reloadInspectedPage, this);
  }

  /**
   * @param {!Common.Event} event
   */
  _reloadInspectedPage(event) {
    var hard = /** @type {boolean} */ (event.data);
    InspectorMain.InspectorMainLate._reloadPage(hard);
  }

  /**
   * @param {boolean} hard
   */
  static _reloadPage(hard) {
    SDK.ResourceTreeModel.reloadAllPages(hard);
  }
};

/**
 * @implements {Protocol.TargetDispatcher}
 */
InspectorMain.BrowserChildTargetManager = class extends SDK.ChildTargetManagerBase {
  /**
   * @param {!SDK.Target} parentTarget
   */
  constructor(parentTarget) {
    super(parentTarget);
    this.targetAgent().invoke_setAutoAttach({autoAttach: true, waitForDebuggerOnStart: true});
  }

  /**
   * @override
   * @return {!Promise}
   */
  suspend() {
    return this.targetAgent().invoke_setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false});
  }

  /**
   * @override
   * @return {!Promise}
   */
  resume() {
    return this.targetAgent().invoke_setAutoAttach({autoAttach: true, waitForDebuggerOnStart: true});
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {string}
   */
  targetName(targetInfo) {
    if (targetInfo.type !== 'iframe') {
      var parsedURL = targetInfo.url.asParsedURL();
      return parsedURL ? parsedURL.lastPathComponentWithFragment() :
                         '#' + (++InspectorMain.BrowserChildTargetManager._lastAnonymousTargetId);
    }
    return super.targetName(targetInfo);
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {number}
   */
  targetCapabilities(targetInfo) {
    if (targetInfo.type === 'worker')
      return SDK.Target.Capability.JS | SDK.Target.Capability.Log | SDK.Target.Capability.Network;


    if (targetInfo.type === 'service_worker')
      return SDK.Target.Capability.Log | SDK.Target.Capability.Network | SDK.Target.Capability.Target;


    if (targetInfo.type === 'iframe') {
      return SDK.Target.Capability.Browser | SDK.Target.Capability.DOM | SDK.Target.Capability.JS |
          SDK.Target.Capability.Log | SDK.Target.Capability.Network | SDK.Target.Capability.Target |
          SDK.Target.Capability.Tracing | SDK.Target.Capability.Emulation | SDK.Target.Capability.Input;
    }
    return 0;
  }

  /**
   * @override
   * @param {!SDK.Target} target
   * @param {boolean} waitingForDebugger
   */
  didAttachToTarget(target, waitingForDebugger) {
    // Only pause the new worker if debugging SW - we are going through the pause on start checkbox.
    if (!target.parentTarget().parentTarget() && Runtime.queryParam('isSharedWorker') && waitingForDebugger) {
      var debuggerModel = target.model(SDK.DebuggerModel);
      if (debuggerModel)
        debuggerModel.pause();
    }
    target.runtimeAgent().runIfWaitingForDebugger();
  }
};

InspectorMain.BrowserChildTargetManager._lastAnonymousTargetId = 0;

/**
 * @implements {Protocol.TargetDispatcher}
 */
InspectorMain.NodeChildTargetManager = class extends SDK.ChildTargetManagerBase {
  /**
   * @param {!SDK.Target} parentTarget
   */
  constructor(parentTarget) {
    super(parentTarget);
    InspectorFrontendHost.setDevicesUpdatesEnabled(true);
    InspectorFrontendHost.events.addEventListener(
        InspectorFrontendHostAPI.Events.DevicesDiscoveryConfigChanged, this._devicesDiscoveryConfigChanged, this);
  }

  /**
   * @param {!Common.Event} event
   */
  _devicesDiscoveryConfigChanged(event) {
    var config = /** @type {!Adb.Config} */ (event.data);
    var locations = [];
    for (var address of config.networkDiscoveryConfig) {
      var parts = address.split(':');
      var port = parseInt(parts[1], 10);
      if (parts[0] && port)
        locations.push({host: parts[0], port: port});
    }
    this.targetAgent().setRemoteLocations(locations);
  }

  /**
   * @override
   */
  dispose() {
    InspectorFrontendHost.events.removeEventListener(
        InspectorFrontendHostAPI.Events.DevicesDiscoveryConfigChanged, this._devicesDiscoveryConfigChanged, this);
    super.dispose();
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   */
  targetCreated(targetInfo) {
    super.targetCreated(targetInfo);
    if (targetInfo.type === 'node' && !targetInfo.attached)
      this.targetAgent().attachToTarget(targetInfo.targetId);
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {string}
   */
  targetName(targetInfo) {
    return Common.UIString('Node.js: %s', targetInfo.url);
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {number}
   */
  targetCapabilities(targetInfo) {
    return SDK.Target.Capability.JS;
  }

  /**
   * @override
   * @param {!SDK.Target} target
   * @param {boolean} waitingForDebugger
   */
  didAttachToTarget(target, waitingForDebugger) {
    target.runtimeAgent().runIfWaitingForDebugger();
  }
};

/**
 * @implements {Protocol.InspectorDispatcher}
 */
InspectorMain.InspectorModel = class extends SDK.SDKModel {
  /**
   * @param {!SDK.Target} target
   */
  constructor(target) {
    super(target);
    target.registerInspectorDispatcher(this);
    target.inspectorAgent().enable();
    this._hideCrashedDialog = null;
  }

  /**
   * @override
   * @param {string} reason
   */
  detached(reason) {
    InspectorMain._disconnectedScreenWithReasonWasShown = true;
    InspectorMain.RemoteDebuggingTerminatedScreen.show(reason);
  }

  /**
   * @override
   */
  targetCrashed() {
    var dialog = new UI.Dialog();
    dialog.setSizeBehavior(UI.GlassPane.SizeBehavior.MeasureContent);
    dialog.addCloseButton();
    dialog.setDimmed(true);
    this._hideCrashedDialog = dialog.hide.bind(dialog);
    new InspectorMain.TargetCrashedScreen(() => this._hideCrashedDialog = null).show(dialog.contentElement);
    dialog.show();
  }

  /**
   * @override;
   */
  targetReloadedAfterCrash() {
    if (this._hideCrashedDialog) {
      this._hideCrashedDialog.call(null);
      this._hideCrashedDialog = null;
    }
  }
};

SDK.SDKModel.register(InspectorMain.InspectorModel, SDK.Target.Capability.Inspector, true);

/**
 * @implements {UI.ActionDelegate}
 * @unrestricted
 */
InspectorMain.ReloadActionDelegate = class {
  /**
   * @override
   * @param {!UI.Context} context
   * @param {string} actionId
   * @return {boolean}
   */
  handleAction(context, actionId) {
    switch (actionId) {
      case 'inspector_main.reload':
        InspectorMain.InspectorMainLate._reloadPage(false);
        return true;
      case 'inspector_main.hard-reload':
        InspectorMain.InspectorMainLate._reloadPage(true);
        return true;
    }
    return false;
  }
};

/**
 * @implements {UI.ToolbarItem.Provider}
 */
InspectorMain.NodeIndicator = class {
  constructor() {
    var element = createElement('div');
    var shadowRoot = UI.createShadowRootWithCoreStyles(element, 'inspector_main/nodeIcon.css');
    this._element = shadowRoot.createChild('div', 'node-icon');
    element.addEventListener('click', () => InspectorFrontendHost.openNodeFrontend(), false);
    this._button = new UI.ToolbarItem(element);
    this._button.setTitle(Common.UIString('Open dedicated DevTools for Node.js'));
    SDK.targetManager.addEventListener(
        SDK.TargetManager.Events.AvailableTargetsChanged,
        event => this._update(/** @type {!Array<!Protocol.Target.TargetInfo>} */ (event.data)));
    this._button.setVisible(false);
    this._update([]);
  }

  /**
   * @param {!Array<!Protocol.Target.TargetInfo>} targetInfos
   */
  _update(targetInfos) {
    var hasNode = !!targetInfos.find(target => target.type === 'node');
    this._element.classList.toggle('inactive', !hasNode);
    if (hasNode)
      this._button.setVisible(true);
  }

  /**
   * @override
   * @return {?UI.ToolbarItem}
   */
  item() {
    return this._button;
  }
};

InspectorMain.NetworkPanelIndicator = class {
  constructor() {
    // TODO: we should not access network from other modules.
    if (!UI.inspectorView.hasPanel('network'))
      return;
    var manager = SDK.multitargetNetworkManager;
    manager.addEventListener(SDK.MultitargetNetworkManager.Events.ConditionsChanged, updateVisibility);
    manager.addEventListener(SDK.MultitargetNetworkManager.Events.BlockedPatternsChanged, updateVisibility);
    manager.addEventListener(SDK.MultitargetNetworkManager.Events.InterceptorsChanged, updateVisibility);
    updateVisibility();

    function updateVisibility() {
      var icon = null;
      if (manager.isThrottling()) {
        icon = UI.Icon.create('smallicon-warning');
        icon.title = Common.UIString('Network throttling is enabled');
      } else if (SDK.multitargetNetworkManager.isIntercepting()) {
        icon = UI.Icon.create('smallicon-warning');
        icon.title = Common.UIString('Requests may be rewritten');
      } else if (manager.isBlocking()) {
        icon = UI.Icon.create('smallicon-warning');
        icon.title = Common.UIString('Requests may be blocked');
      }
      UI.inspectorView.setPanelIcon('network', icon);
    }
  }
};

/**
 * @unrestricted
 */
InspectorMain.SourcesPanelIndicator = class {
  constructor() {
    Common.moduleSetting('javaScriptDisabled').addChangeListener(javaScriptDisabledChanged);
    javaScriptDisabledChanged();

    function javaScriptDisabledChanged() {
      var icon = null;
      var javaScriptDisabled = Common.moduleSetting('javaScriptDisabled').get();
      if (javaScriptDisabled) {
        icon = UI.Icon.create('smallicon-warning');
        icon.title = Common.UIString('JavaScript is disabled');
      }
      UI.inspectorView.setPanelIcon('sources', icon);
    }
  }
};

/**
 * @unrestricted
 */
InspectorMain.InspectedNodeRevealer = class {
  constructor() {
    SDK.targetManager.addModelListener(
        SDK.OverlayModel, SDK.OverlayModel.Events.InspectNodeRequested, this._inspectNode, this);
  }

  /**
   * @param {!Common.Event} event
   */
  _inspectNode(event) {
    var deferredNode = /** @type {!SDK.DeferredDOMNode} */ (event.data);
    Common.Revealer.reveal(deferredNode);
  }
};

/**
 * @unrestricted
 */
InspectorMain.RemoteDebuggingTerminatedScreen = class extends UI.VBox {
  /**
   * @param {string} reason
   */
  constructor(reason) {
    super(true);
    this.registerRequiredCSS('inspector_main/remoteDebuggingTerminatedScreen.css');
    var message = this.contentElement.createChild('div', 'message');
    message.createChild('span').textContent = Common.UIString('Debugging connection was closed. Reason: ');
    message.createChild('span', 'reason').textContent = reason;
    this.contentElement.createChild('div', 'message').textContent =
        Common.UIString('Reconnect when ready by reopening DevTools.');
    var button = UI.createTextButton(Common.UIString('Reconnect DevTools'), () => window.location.reload());
    this.contentElement.createChild('div', 'button').appendChild(button);
  }

  /**
   * @param {string} reason
   */
  static show(reason) {
    var dialog = new UI.Dialog();
    dialog.setSizeBehavior(UI.GlassPane.SizeBehavior.MeasureContent);
    dialog.addCloseButton();
    dialog.setDimmed(true);
    new InspectorMain.RemoteDebuggingTerminatedScreen(reason).show(dialog.contentElement);
    dialog.show();
  }
};

/**
 * @unrestricted
 */
InspectorMain.TargetCrashedScreen = class extends UI.VBox {
  /**
   * @param {function()} hideCallback
   */
  constructor(hideCallback) {
    super(true);
    this.registerRequiredCSS('inspector_main/targetCrashedScreen.css');
    this.contentElement.createChild('div', 'message').textContent =
        Common.UIString('DevTools was disconnected from the page.');
    this.contentElement.createChild('div', 'message').textContent =
        Common.UIString('Once page is reloaded, DevTools will automatically reconnect.');
    this._hideCallback = hideCallback;
  }

  /**
   * @override
   */
  willHide() {
    this._hideCallback.call(null);
  }
};


/**
 * @implements {SDK.TargetManager.Observer}
 * @unrestricted
 */
InspectorMain.BackendSettingsSync = class {
  constructor() {
    this._autoAttachSetting = Common.settings.moduleSetting('autoAttachToCreatedPages');
    this._autoAttachSetting.addChangeListener(this._updateAutoAttach, this);
    this._updateAutoAttach();

    this._adBlockEnabledSetting = Common.settings.moduleSetting('network.adBlockingEnabled');
    this._adBlockEnabledSetting.addChangeListener(this._update, this);

    SDK.targetManager.observeTargets(this, SDK.Target.Capability.Browser);
  }

  /**
   * @param {!SDK.Target} target
   */
  _updateTarget(target) {
    target.pageAgent().setAdBlockingEnabled(this._adBlockEnabledSetting.get());
  }

  _updateAutoAttach() {
    InspectorFrontendHost.setOpenNewWindowForPopups(this._autoAttachSetting.get());
  }

  _update() {
    SDK.targetManager.targets(SDK.Target.Capability.Browser).forEach(this._updateTarget, this);
  }

  /**
   * @param {!SDK.Target} target
   * @override
   */
  targetAdded(target) {
    this._updateTarget(target);
  }

  /**
   * @param {!SDK.Target} target
   * @override
   */
  targetRemoved(target) {
  }
};
