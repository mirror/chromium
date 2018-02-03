/*
 * Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

SDK.TargetManager = class extends Common.Object {
  constructor() {
    super();
    /** @type {!Array.<!SDK.Target>} */
    this._targets = [];
    /** @type {!Array.<!SDK.TargetManager.Observer>} */
    this._observers = [];
    this._observerCapabiliesMaskSymbol = Symbol('observerCapabilitiesMask');
    /** @type {!Map<symbol, !Array<{modelClass: !Function, thisObject: (!Object|undefined), listener: function(!Common.Event)}>>} */
    this._modelListeners = new Map();
    /** @type {!Map<function(new:SDK.SDKModel, !SDK.Target), !Array<!SDK.SDKModelObserver>>} */
    this._modelObservers = new Map();
    this._isSuspended = false;
    /** @type {!Map<!SDK.Target, !SDK.ChildTargetManagerBase>} */
    this._childTargetManagers = new Map();
    /** @type {!Protocol.InspectorBackend.Connection} */
    this._mainConnection;
    /** @type {function()} */
    this._webSocketConnectionLostCallback;
  }

  /**
   * @return {!Promise}
   */
  suspendAllTargets() {
    if (this._isSuspended)
      return Promise.resolve();
    this._isSuspended = true;
    this.dispatchEventToListeners(SDK.TargetManager.Events.SuspendStateChanged);

    var promises = [];
    for (var target of this._targets) {
      var childTargetManager = this._childTargetManagers.get(target);
      if (childTargetManager)
        promises.push(childTargetManager.suspend());
      promises.push(target.suspend());
    }
    return Promise.all(promises);
  }

  /**
   * @return {!Promise}
   */
  resumeAllTargets() {
    if (!this._isSuspended)
      return Promise.resolve();
    this._isSuspended = false;
    this.dispatchEventToListeners(SDK.TargetManager.Events.SuspendStateChanged);

    var promises = [];
    for (var target of this._targets) {
      var childTargetManager = this._childTargetManagers.get(target);
      if (childTargetManager)
        promises.push(childTargetManager.resume());
      promises.push(target.resume());
    }
    return Promise.all(promises);
  }

  /**
   * @return {boolean}
   */
  allTargetsSuspended() {
    return this._isSuspended;
  }

  /**
   * @param {function(new:T,!SDK.Target)} modelClass
   * @return {!Array<!T>}
   * @template T
   */
  models(modelClass) {
    var result = [];
    for (var i = 0; i < this._targets.length; ++i) {
      var model = this._targets[i].model(modelClass);
      if (model)
        result.push(model);
    }
    return result;
  }

  /**
   * @return {string}
   */
  inspectedURL() {
    return this._targets[0] ? this._targets[0].inspectedURL() : '';
  }

  /**
   * @param {function(new:T,!SDK.Target)} modelClass
   * @param {!SDK.SDKModelObserver<T>} observer
   * @template T
   */
  observeModels(modelClass, observer) {
    if (!this._modelObservers.has(modelClass))
      this._modelObservers.set(modelClass, []);
    this._modelObservers.get(modelClass).push(observer);
    for (var model of this.models(modelClass))
      observer.modelAdded(model);
  }

  /**
   * @param {function(new:T,!SDK.Target)} modelClass
   * @param {!SDK.SDKModelObserver<T>} observer
   * @template T
   */
  unobserveModels(modelClass, observer) {
    if (!this._modelObservers.has(modelClass))
      return;
    var observers = this._modelObservers.get(modelClass);
    observers.remove(observer);
    if (!observers.length)
      this._modelObservers.delete(modelClass);
  }

  /**
   * @param {!SDK.Target} target
   * @param {function(new:SDK.SDKModel,!SDK.Target)} modelClass
   * @param {!SDK.SDKModel} model
   */
  modelAdded(target, modelClass, model) {
    if (!this._modelObservers.has(modelClass))
      return;
    for (var observer of this._modelObservers.get(modelClass).slice())
      observer.modelAdded(model);
  }

  /**
   * @param {!SDK.Target} target
   * @param {function(new:SDK.SDKModel,!SDK.Target)} modelClass
   * @param {!SDK.SDKModel} model
   */
  _modelRemoved(target, modelClass, model) {
    if (!this._modelObservers.has(modelClass))
      return;
    for (var observer of this._modelObservers.get(modelClass).slice())
      observer.modelRemoved(model);
  }

  /**
   * @param {!Function} modelClass
   * @param {symbol} eventType
   * @param {function(!Common.Event)} listener
   * @param {!Object=} thisObject
   */
  addModelListener(modelClass, eventType, listener, thisObject) {
    for (var i = 0; i < this._targets.length; ++i) {
      var model = this._targets[i].model(modelClass);
      if (model)
        model.addEventListener(eventType, listener, thisObject);
    }
    if (!this._modelListeners.has(eventType))
      this._modelListeners.set(eventType, []);
    this._modelListeners.get(eventType).push({modelClass: modelClass, thisObject: thisObject, listener: listener});
  }

  /**
   * @param {!Function} modelClass
   * @param {symbol} eventType
   * @param {function(!Common.Event)} listener
   * @param {!Object=} thisObject
   */
  removeModelListener(modelClass, eventType, listener, thisObject) {
    if (!this._modelListeners.has(eventType))
      return;

    for (var i = 0; i < this._targets.length; ++i) {
      var model = this._targets[i].model(modelClass);
      if (model)
        model.removeEventListener(eventType, listener, thisObject);
    }

    var listeners = this._modelListeners.get(eventType);
    for (var i = 0; i < listeners.length; ++i) {
      if (listeners[i].modelClass === modelClass && listeners[i].listener === listener &&
          listeners[i].thisObject === thisObject)
        listeners.splice(i--, 1);
    }
    if (!listeners.length)
      this._modelListeners.delete(eventType);
  }

  /**
   * @param {!SDK.TargetManager.Observer} targetObserver
   * @param {number=} capabilitiesMask
   */
  observeTargets(targetObserver, capabilitiesMask) {
    if (this._observerCapabiliesMaskSymbol in targetObserver)
      throw new Error('Observer can only be registered once');
    targetObserver[this._observerCapabiliesMaskSymbol] = capabilitiesMask || 0;
    this.targets(capabilitiesMask).forEach(targetObserver.targetAdded.bind(targetObserver));
    this._observers.push(targetObserver);
  }

  /**
   * @param {!SDK.TargetManager.Observer} targetObserver
   */
  unobserveTargets(targetObserver) {
    delete targetObserver[this._observerCapabiliesMaskSymbol];
    this._observers.remove(targetObserver);
  }

  /**
   * @param {string} id
   * @param {string} name
   * @param {number} capabilitiesMask
   * @param {!Protocol.InspectorBackend.Connection.Factory} connectionFactory
   * @param {?SDK.Target} parentTarget
   * @return {!SDK.Target}
   */
  createTarget(id, name, capabilitiesMask, connectionFactory, parentTarget) {
    var target = new SDK.Target(this, id, name, capabilitiesMask, connectionFactory, parentTarget, this._isSuspended);
    target.createModels(new Set(this._modelObservers.keys()));
    if (target.hasTargetCapability()) {
      if (Runtime.queryParam('nodeFrontend'))
        this._childTargetManagers.set(target, new SDK.NodeChildTargetManager(this, target));
      else
        this._childTargetManagers.set(target, new SDK.BrowserChildTargetManager(this, target));
    }
    this._targets.push(target);

    var copy = this._observersForTarget(target);
    for (var i = 0; i < copy.length; ++i)
      copy[i].targetAdded(target);

    for (var modelClass of target.models().keys())
      this.modelAdded(target, modelClass, target.models().get(modelClass));

    for (var pair of this._modelListeners) {
      var listeners = pair[1];
      for (var i = 0; i < listeners.length; ++i) {
        var model = target.model(listeners[i].modelClass);
        if (model)
          model.addEventListener(/** @type {symbol} */ (pair[0]), listeners[i].listener, listeners[i].thisObject);
      }
    }

    return target;
  }

  /**
   * @param {!SDK.Target} target
   * @return {!Array<!SDK.TargetManager.Observer>}
   */
  _observersForTarget(target) {
    return this._observers.filter(
        observer => target.hasAllCapabilities(observer[this._observerCapabiliesMaskSymbol] || 0));
  }

  /**
   * @param {!SDK.Target} target
   */
  removeTarget(target) {
    if (!this._targets.includes(target))
      return;

    var childTargetManager = this._childTargetManagers.get(target);
    this._childTargetManagers.delete(target);
    if (childTargetManager)
      childTargetManager.dispose();

    this._targets.remove(target);

    for (var modelClass of target.models().keys())
      this._modelRemoved(target, modelClass, target.models().get(modelClass));

    var copy = this._observersForTarget(target);
    for (var i = 0; i < copy.length; ++i)
      copy[i].targetRemoved(target);

    for (var pair of this._modelListeners) {
      var listeners = pair[1];
      for (var i = 0; i < listeners.length; ++i) {
        var model = target.model(listeners[i].modelClass);
        if (model)
          model.removeEventListener(/** @type {symbol} */ (pair[0]), listeners[i].listener, listeners[i].thisObject);
      }
    }
  }

  /**
   * @param {number=} capabilitiesMask
   * @return {!Array.<!SDK.Target>}
   */
  targets(capabilitiesMask) {
    if (!capabilitiesMask)
      return this._targets.slice();
    else
      return this._targets.filter(target => target.hasAllCapabilities(capabilitiesMask || 0));
  }

  /**
   *
   * @param {string} id
   * @return {?SDK.Target}
   */
  targetById(id) {
    // TODO(dgozman): add a map id -> target.
    for (var i = 0; i < this._targets.length; ++i) {
      if (this._targets[i].id() === id)
        return this._targets[i];
    }
    return null;
  }

  /**
   * @return {?SDK.Target}
   */
  mainTarget() {
    return this._targets[0] || null;
  }

  /**
   * @param {function()} webSocketConnectionLostCallback
   */
  connectToMainTarget(webSocketConnectionLostCallback) {
    this._webSocketConnectionLostCallback = webSocketConnectionLostCallback;
    this._connectAndCreateMainTarget();
  }

  _connectAndCreateMainTarget() {
    if (Runtime.queryParam('nodeFrontend'))
      Host.userMetrics.actionTaken(Host.UserMetrics.Action.ConnectToNodeJSFromFrontend);
    else if (Runtime.queryParam('v8only'))
      Host.userMetrics.actionTaken(Host.UserMetrics.Action.ConnectToNodeJSDirectly);

    var target = this.createTarget(
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
  interceptMainConnection(onMessage) {
    var params = {onMessage: onMessage, onDisconnect: this._connectAndCreateMainTarget.bind(this)};
    return this._mainConnection.disconnect().then(this._createMainConnection.bind(this, params));
  }
};

/**
 * @implements {Protocol.TargetDispatcher}
 */
SDK.ChildTargetManagerBase = class {
  /**
   * @param {!SDK.TargetManager} targetManager
   * @param {!SDK.Target} parentTarget
   */
  constructor(targetManager, parentTarget) {
    this._targetManager = targetManager;
    this._parentTarget = parentTarget;
    this._targetAgent = parentTarget.targetAgent();
    /** @type {!Map<string, !Protocol.Target.TargetInfo>} */
    this._targetInfos = new Map();
    /** @type {!Map<string, !SDK.ChildConnection>} */
    this._childConnections = new Map();
    parentTarget.registerTargetDispatcher(this);
    if (!parentTarget.parentTarget())
      this._targetAgent.setDiscoverTargets(true);
  }

  /**
   * @return {!Promise}
   */
  suspend() {
    return Promise.resolve();
  }

  /**
   * @return {!Promise}
   */
  resume() {
    return Promise.resolve();
  }

  dispose() {
    for (var sessionId of this._childConnections.keys())
      this.detachedFromTarget(sessionId, undefined);
  }

  /**
   * @return {!Protocol.TargetAgent}
   */
  targetAgent() {
    return this._targetAgent;
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   */
  targetCreated(targetInfo) {
    this._targetInfos.set(targetInfo.targetId, targetInfo);
    this._fireAvailableTargetsChanged();
  }

  /**
   * @override
   * @param {!Protocol.Target.TargetInfo} targetInfo
   */
  targetInfoChanged(targetInfo) {
    this._targetInfos.set(targetInfo.targetId, targetInfo);
    this._fireAvailableTargetsChanged();
  }

  /**
   * @override
   * @param {string} targetId
   */
  targetDestroyed(targetId) {
    this._targetInfos.delete(targetId);
    this._fireAvailableTargetsChanged();
  }

  _fireAvailableTargetsChanged() {
    this._targetManager.dispatchEventToListeners(
        SDK.TargetManager.Events.AvailableTargetsChanged, this._targetInfos.valuesArray());
  }

  /**
   * @override
   * @param {string} sessionId
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @param {boolean} waitingForDebugger
   */
  attachedToTarget(sessionId, targetInfo, waitingForDebugger) {
    var target = this._targetManager.createTarget(
        targetInfo.targetId, this.targetName(targetInfo), this.targetCapabilities(targetInfo),
        this._createChildConnection.bind(this, this._targetAgent, sessionId), this._parentTarget);
    this.didAttachToTarget(target, waitingForDebugger);
  }

  /**
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {string}
   */
  targetName(targetInfo) {
    var parsedURL = targetInfo.url.asParsedURL();
    return parsedURL ? parsedURL.lastPathComponentWithFragment() : targetInfo.url.trimMiddle(30);
  }

  /**
   * @param {!Protocol.Target.TargetInfo} targetInfo
   * @return {number}
   */
  targetCapabilities(targetInfo) {
    return 0;
  }

  /**
   * @param {!SDK.Target} target
   * @param {boolean} waitingForDebugger
   */
  didAttachToTarget(target, waitingForDebugger) {
  }

  /**
   * @override
   * @param {string} sessionId
   * @param {string=} childTargetId
   */
  detachedFromTarget(sessionId, childTargetId) {
    this._childConnections.get(sessionId)._onDisconnect.call(null, 'target terminated');
    this._childConnections.delete(sessionId);
  }

  /**
   * @override
   * @param {string} sessionId
   * @param {string} message
   * @param {string=} childTargetId
   */
  receivedMessageFromTarget(sessionId, message, childTargetId) {
    var connection = this._childConnections.get(sessionId);
    if (connection)
      connection._onMessage.call(null, message);
  }

  /**
   * @param {!Protocol.TargetAgent} agent
   * @param {string} sessionId
   * @param {!Protocol.InspectorBackend.Connection.Params} params
   * @return {!Protocol.InspectorBackend.Connection}
   */
  _createChildConnection(agent, sessionId, params) {
    var connection = new SDK.ChildConnection(agent, sessionId, params);
    this._childConnections.set(sessionId, connection);
    return connection;
  }
};

/**
 * @implements {Protocol.TargetDispatcher}
 */
SDK.BrowserChildTargetManager = class extends SDK.ChildTargetManagerBase {
  /**
   * @param {!SDK.TargetManager} targetManager
   * @param {!SDK.Target} parentTarget
   */
  constructor(targetManager, parentTarget) {
    super(targetManager, parentTarget);
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
                         '#' + (++SDK.BrowserChildTargetManager._lastAnonymousTargetId);
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

SDK.BrowserChildTargetManager._lastAnonymousTargetId = 0;

/**
 * @implements {Protocol.TargetDispatcher}
 */
SDK.NodeChildTargetManager = class extends SDK.ChildTargetManagerBase {
  /**
   * @param {!SDK.TargetManager} targetManager
   * @param {!SDK.Target} parentTarget
   */
  constructor(targetManager, parentTarget) {
    super(targetManager, parentTarget);
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
 * @implements {Protocol.InspectorBackend.Connection}
 */
SDK.ChildConnection = class {
  /**
   * @param {!Protocol.TargetAgent} agent
   * @param {string} sessionId
   * @param {!Protocol.InspectorBackend.Connection.Params} params
   */
  constructor(agent, sessionId, params) {
    this._agent = agent;
    this._sessionId = sessionId;
    this._onMessage = params.onMessage;
    this._onDisconnect = params.onDisconnect;
  }

  /**
   * @override
   * @param {string} message
   */
  sendMessage(message) {
    this._agent.sendMessageToTarget(message, this._sessionId);
  }

  /**
   * @override
   * @return {!Promise}
   */
  disconnect() {
    throw 'Not implemented';
  }
};

/** @enum {symbol} */
SDK.TargetManager.Events = {
  InspectedURLChanged: Symbol('InspectedURLChanged'),
  NameChanged: Symbol('NameChanged'),
  SuspendStateChanged: Symbol('SuspendStateChanged'),
  AvailableTargetsChanged: Symbol('AvailableTargetsChanged')
};

/**
 * @interface
 */
SDK.TargetManager.Observer = function() {};

SDK.TargetManager.Observer.prototype = {
  /**
   * @param {!SDK.Target} target
   */
  targetAdded(target) {},

  /**
   * @param {!SDK.Target} target
   */
  targetRemoved(target) {},
};

/**
 * @interface
 * @template T
 */
SDK.SDKModelObserver = function() {};

SDK.SDKModelObserver.prototype = {
  /**
   * @param {!T} model
   */
  modelAdded(model) {},

  /**
   * @param {!T} model
   */
  modelRemoved(model) {},
};

/**
 * @type {!SDK.TargetManager}
 */
SDK.targetManager = new SDK.TargetManager();
