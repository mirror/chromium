// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Class handling creation and teardown of a remoting client session.
 *
 * The ClientSession class controls lifetime of the client plugin
 * object and provides the plugin with the functionality it needs to
 * establish connection. Specifically it:
 *  - Delivers incoming/otgoing signaling messages,
 *  - Adjusts plugin size and position when destop resolution changes,
 *
 * This class should not access the plugin directly, instead it should
 * do it through ClientPlugin class which abstracts plugin version
 * differences.
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * @param {string} hostJid The jid of the host to connect to.
 * @param {string} hostPublicKey The base64 encoded version of the host's
 *     public key.
 * @param {string} sharedSecret The access code for IT2Me or the PIN
 *     for Me2Me.
 * @param {string} authenticationMethods Comma-separated list of
 *     authentication methods the client should attempt to use.
 * @param {string} authenticationTag A host-specific tag to mix into
 *     authentication hashes.
 * @param {string} email The username for the talk network.
 * @param {remoting.ClientSession.Mode} mode The mode of this connection.
 * @param {function(remoting.ClientSession.State,
                    remoting.ClientSession.State):void} onStateChange
 *     The callback to invoke when the session changes state.
 * @constructor
 */
remoting.ClientSession = function(hostJid, hostPublicKey, sharedSecret,
                                  authenticationMethods, authenticationTag,
                                  email, mode, onStateChange) {
  this.state = remoting.ClientSession.State.CREATED;

  this.hostJid = hostJid;
  this.hostPublicKey = hostPublicKey;
  this.sharedSecret = sharedSecret;
  this.authenticationMethods = authenticationMethods;
  this.authenticationTag = authenticationTag;
  this.email = email;
  this.mode = mode;
  this.clientJid = '';
  this.sessionId = '';
  /** @type {remoting.ClientPlugin} */
  this.plugin = null;
  this.scaleToFit = false;
  this.logToServer = new remoting.LogToServer();
  this.onStateChange = onStateChange;
  /** @type {remoting.ClientSession} */
  var that = this;
  /** @type {function():void} @private */
  this.refocusPlugin_ = function() { that.plugin.element().focus(); };
};

// Note that the positive values in both of these enums are copied directly
// from chromoting_scriptable_object.h and must be kept in sync. The negative
// values represent states transitions that occur within the web-app that have
// no corresponding plugin state transition.
/** @enum {number} */
remoting.ClientSession.State = {
  CREATED: -3,
  BAD_PLUGIN_VERSION: -2,
  UNKNOWN_PLUGIN_ERROR: -1,
  UNKNOWN: 0,
  CONNECTING: 1,
  INITIALIZING: 2,
  CONNECTED: 3,
  CLOSED: 4,
  FAILED: 5
};

/** @enum {number} */
remoting.ClientSession.ConnectionError = {
  NONE: 0,
  HOST_IS_OFFLINE: 1,
  SESSION_REJECTED: 2,
  INCOMPATIBLE_PROTOCOL: 3,
  NETWORK_FAILURE: 4
};

// The mode of this session.
/** @enum {number} */
remoting.ClientSession.Mode = {
  IT2ME: 0,
  ME2ME: 1
};

/**
 * Type used for performance statistics collected by the plugin.
 * @constructor
 */
remoting.ClientSession.PerfStats = function() {};
/** @type {number} */
remoting.ClientSession.PerfStats.prototype.videoBandwidth;
/** @type {number} */
remoting.ClientSession.PerfStats.prototype.videoFrameRate;
/** @type {number} */
remoting.ClientSession.PerfStats.prototype.captureLatency;
/** @type {number} */
remoting.ClientSession.PerfStats.prototype.encodeLatency;
/** @type {number} */
remoting.ClientSession.PerfStats.prototype.decodeLatency;
/** @type {number} */
remoting.ClientSession.PerfStats.prototype.renderLatency;
/** @type {number} */
remoting.ClientSession.PerfStats.prototype.roundtripLatency;

// Keys for connection statistics.
remoting.ClientSession.STATS_KEY_VIDEO_BANDWIDTH = 'videoBandwidth';
remoting.ClientSession.STATS_KEY_VIDEO_FRAME_RATE = 'videoFrameRate';
remoting.ClientSession.STATS_KEY_CAPTURE_LATENCY = 'captureLatency';
remoting.ClientSession.STATS_KEY_ENCODE_LATENCY = 'encodeLatency';
remoting.ClientSession.STATS_KEY_DECODE_LATENCY = 'decodeLatency';
remoting.ClientSession.STATS_KEY_RENDER_LATENCY = 'renderLatency';
remoting.ClientSession.STATS_KEY_ROUNDTRIP_LATENCY = 'roundtripLatency';

/**
 * The current state of the session.
 * @type {remoting.ClientSession.State}
 */
remoting.ClientSession.prototype.state = remoting.ClientSession.State.UNKNOWN;

/**
 * The last connection error. Set when state is set to FAILED.
 * @type {remoting.ClientSession.ConnectionError}
 */
remoting.ClientSession.prototype.error =
    remoting.ClientSession.ConnectionError.NONE;

/**
 * The id of the client plugin
 *
 * @const
 */
remoting.ClientSession.prototype.PLUGIN_ID = 'session-client-plugin';

/**
 * Callback to invoke when the state is changed.
 *
 * @param {remoting.ClientSession.State} oldState The previous state.
 * @param {remoting.ClientSession.State} newState The current state.
 */
remoting.ClientSession.prototype.onStateChange =
    function(oldState, newState) { };

/**
 * @param {Element} container The element to add the plugin to.
 * @param {string} id Id to use for the plugin element .
 * @return {remoting.ClientPlugin} Create plugin object for the locally
 * installed plugin.
 */
remoting.ClientSession.prototype.createClientPlugin_ = function(container, id) {
  var plugin = /** @type {remoting.ViewerPlugin} */
      document.createElement('embed');

  plugin.id = id;
  plugin.src = 'about://none';
  plugin.type = 'application/vnd.chromium.remoting-viewer';
  plugin.width = 0;
  plugin.height = 0;
  plugin.tabIndex = 0;  // Required, otherwise focus() doesn't work.
  container.appendChild(plugin);

  // Previous versions of the plugin didn't support messaging-based
  // interface. They can be identified by presence of apiVersion
  // property that is less than 5.
  if (plugin.apiVersion && plugin.apiVersion < 5) {
    return new remoting.ClientPluginV1(plugin);
  } else {
    return new remoting.ClientPluginAsync(plugin);
  }
};

/**
 * Adds <embed> element to |container| and readies the sesion object.
 *
 * @param {Element} container The element to add the plugin to.
 * @param {string} oauth2AccessToken A valid OAuth2 access token.
 */
remoting.ClientSession.prototype.createPluginAndConnect =
    function(container, oauth2AccessToken) {
  this.plugin = this.createClientPlugin_(container, this.PLUGIN_ID);

  this.plugin.element().focus();
  this.plugin.element().addEventListener('blur', this.refocusPlugin_, false);

  /** @type {remoting.ClientSession} */
  var that = this;
  /** @param {boolean} result */
  this.plugin.initialize(function(result) {
      that.onPluginInitialized_(oauth2AccessToken, result);
    });
};

/**
 * @param {string} oauth2AccessToken
 * @param {boolean} initialized
 */
remoting.ClientSession.prototype.onPluginInitialized_ =
    function(oauth2AccessToken, initialized) {
  if (!initialized) {
    remoting.debug.log('ERROR: remoting plugin not loaded');
    this.plugin.cleanup();
    delete this.plugin;
    this.setState_(remoting.ClientSession.State.UNKNOWN_PLUGIN_ERROR);
    return;
  }

  if (!this.plugin.isSupportedVersion()) {
    this.plugin.cleanup();
    delete this.plugin;
    this.setState_(remoting.ClientSession.State.BAD_PLUGIN_VERSION);
    return;
  }

  // Enable scale-to-fit if the plugin is new enough for high-quality scaling.
  this.setScaleToFit(this.plugin.isHiQualityScalingSupported());

  /** @type {remoting.ClientSession} */ var that = this;
  /** @param {string} msg The IQ stanza to send. */
  this.plugin.onOutgoingIqHandler = function(msg) { that.sendIq_(msg); };
  /** @param {string} msg The message to log. */
  this.plugin.onDebugMessageHandler = function(msg) {
    remoting.debug.log('plugin: ' + msg);
  };

  /**
   * @param {number} status The plugin status.
   * @param {number} error The plugin error status, if any.
   */
  this.plugin.onConnectionStatusUpdateHandler = function(status, error) {
    that.connectionStatusUpdateCallback(status, error);
  };
  this.plugin.onDesktopSizeUpdateHandler = function() {
    that.onDesktopSizeChanged_();
  };

  this.connectPluginToWcs_(oauth2AccessToken);
};

/**
 * Deletes the <embed> element from the container, without sending a
 * session_terminate request.  This is to be called when the session was
 * disconnected by the Host.
 *
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.removePlugin = function() {
  if (this.plugin) {
    this.plugin.element().removeEventListener(
        'blur', this.refocusPlugin_, false);
    this.plugin.cleanup();
    this.plugin = null;
  }
};

/**
 * Deletes the <embed> element from the container and disconnects.
 *
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.disconnect = function() {
  // The plugin won't send a state change notification, so we explicitly log
  // the fact that the connection has closed.
  this.logToServer.logClientSessionStateChange(
      remoting.ClientSession.State.CLOSED,
      remoting.ClientSession.ConnectionError.NONE, this.mode);
  if (remoting.wcs) {
    remoting.wcs.setOnIq(function(stanza) {});
    this.sendIq_(
        '<cli:iq ' +
            'to="' + this.hostJid + '" ' +
            'type="set" ' +
            'id="session-terminate" ' +
            'xmlns:cli="jabber:client">' +
          '<jingle ' +
              'xmlns="urn:xmpp:jingle:1" ' +
              'action="session-terminate" ' +
              'initiator="' + this.clientJid + '" ' +
              'sid="' + this.sessionId + '">' +
            '<reason><success/></reason>' +
          '</jingle>' +
        '</cli:iq>');
  }
  this.removePlugin();
};

/**
 * Enables or disables the client's scale-to-fit feature.
 *
 * @param {boolean} scaleToFit True to enable scale-to-fit, false otherwise.
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.setScaleToFit = function(scaleToFit) {
  this.scaleToFit = scaleToFit;
  this.updateDimensions();
}

/**
 * Returns whether the client is currently scaling the host to fit the tab.
 *
 * @return {boolean} The current scale-to-fit setting.
 */
remoting.ClientSession.prototype.getScaleToFit = function() {
  return this.scaleToFit;
}

/**
 * Sends an IQ stanza via the http xmpp proxy.
 *
 * @private
 * @param {string} msg XML string of IQ stanza to send to server.
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.sendIq_ = function(msg) {
  remoting.debug.logIq(true, msg);
  // Extract the session id, so we can close the session later.
  var parser = new DOMParser();
  var iqNode = parser.parseFromString(msg, 'text/xml').firstChild;
  var jingleNode = iqNode.firstChild;
  if (jingleNode) {
    var action = jingleNode.getAttribute('action');
    if (jingleNode.nodeName == 'jingle' && action == 'session-initiate') {
      this.sessionId = jingleNode.getAttribute('sid');
    }
  }

  // Send the stanza.
  if (remoting.wcs) {
    remoting.wcs.sendIq(msg);
  } else {
    remoting.debug.log('Tried to send IQ before WCS was ready.');
    this.setState_(remoting.ClientSession.State.FAILED);
  }
};

/**
 * Connects the plugin to WCS.
 *
 * @private
 * @param {string} oauth2AccessToken A valid OAuth2 access token.
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.connectPluginToWcs_ =
    function(oauth2AccessToken) {
  this.clientJid = remoting.wcs.getJid();
  if (this.clientJid == '') {
    remoting.debug.log('Tried to connect without a full JID.');
  }
  remoting.debug.setJids(this.clientJid, this.hostJid);
  /** @type {remoting.ClientSession} */
  var that = this;
  /** @param {string} stanza The IQ stanza received. */
  var onIncomingIq = function(stanza) {
    remoting.debug.logIq(false, stanza);
    that.plugin.onIncomingIq(stanza);
  }
  remoting.wcs.setOnIq(onIncomingIq);
  that.plugin.connect(this.hostJid, this.hostPublicKey, this.clientJid,
                      this.sharedSecret, this.authenticationMethods,
                      this.authenticationTag);
};

/**
 * Callback that the plugin invokes to indicate that the connection
 * status has changed.
 *
 * @param {number} status The plugin's status.
 * @param {number} error The plugin's error state, if any.
 */
remoting.ClientSession.prototype.connectionStatusUpdateCallback =
    function(status, error) {
  if (status == remoting.ClientSession.State.CONNECTED) {
    this.onDesktopSizeChanged_();
  } else if (status == remoting.ClientSession.State.FAILED) {
    this.error = /** @type {remoting.ClientSession.ConnectionError} */ (error);
  }
  this.setState_(/** @type {remoting.ClientSession.State} */ (status));
};

/**
 * @private
 * @param {remoting.ClientSession.State} newState The new state for the session.
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.setState_ = function(newState) {
  var oldState = this.state;
  this.state = newState;
  if (this.onStateChange) {
    this.onStateChange(oldState, newState);
  }
  this.logToServer.logClientSessionStateChange(this.state, this.error,
      this.mode);
};

/**
 * This is a callback that gets called when the window is resized.
 *
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.onResize = function() {
  this.updateDimensions();
};

/**
 * This is a callback that gets called when the plugin notifies us of a change
 * in the size of the remote desktop.
 *
 * @private
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.onDesktopSizeChanged_ = function() {
  remoting.debug.log('desktop size changed: ' +
                     this.plugin.desktopWidth + 'x' +
                     this.plugin.desktopHeight);
  this.updateDimensions();
};

/**
 * Refreshes the plugin's dimensions, taking into account the sizes of the
 * remote desktop and client window, and the current scale-to-fit setting.
 *
 * @return {void} Nothing.
 */
remoting.ClientSession.prototype.updateDimensions = function() {
  if (this.plugin.desktopWidth == 0 ||
      this.plugin.desktopHeight == 0) {
    return;
  }

  var windowWidth = window.innerWidth;
  var windowHeight = window.innerHeight;
  var scale = 1.0;

  if (this.getScaleToFit()) {
    var scaleFitWidth = 1.0 * windowWidth / this.plugin.desktopWidth;
    var scaleFitHeight = 1.0 * windowHeight / this.plugin.desktopHeight;
    scale = Math.min(1.0, scaleFitHeight, scaleFitWidth);
  }

  var width = this.plugin.desktopWidth * scale;
  var height = this.plugin.desktopHeight * scale;

  // Resize the plugin if necessary.
  this.plugin.element().width = width;
  this.plugin.element().height = height;

  // Position the container.
  // TODO(wez): We should take into account scrollbars when positioning.
  var parentNode = this.plugin.element().parentNode;

  if (width < windowWidth) {
    parentNode.style.left = (windowWidth - width) / 2 + 'px';
  } else {
    parentNode.style.left = '0';
  }

  if (height < windowHeight) {
    parentNode.style.top = (windowHeight - height) / 2 + 'px';
  } else {
    parentNode.style.top = '0';
  }

  remoting.debug.log('plugin dimensions: ' +
                     parentNode.style.left + ',' +
                     parentNode.style.top + '-' +
                     width + 'x' + height + '.');
  this.plugin.setScaleToFit(this.getScaleToFit());
};

/**
 * Returns an associative array with a set of stats for this connection.
 *
 * @return {remoting.ClientSession.PerfStats} The connection statistics.
 */
remoting.ClientSession.prototype.getPerfStats = function() {
  return this.plugin.getPerfStats();
};

/**
 * Logs statistics.
 *
 * @param {remoting.ClientSession.PerfStats} stats
 */
remoting.ClientSession.prototype.logStatistics = function(stats) {
  this.logToServer.logStatistics(stats, this.mode);
};
