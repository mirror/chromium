// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * A signal strategy encapsulating a primary and a back-up strategy. If the
 * primary fails or times out, then the secondary is used. Information about
 * which strategy was used, and why, is returned via |onProgressCallback|.
 *
 * @param {function(
 *             function(remoting.SignalStrategy.State)
 *         ):remoting.SignalStrategy} primaryFactory
 * @param {function(
 *             function(remoting.SignalStrategy.State)
 *         ):remoting.SignalStrategy} secondaryFactory
 * @param {function(remoting.SignalStrategy.State):void} onStateChangedCallback
 * @param {function(remoting.FallbackSignalStrategy.Progress)}
 *     onProgressCallback
 *
 * @implements {remoting.SignalStrategy}
 * @constructor
 */
remoting.FallbackSignalStrategy = function(
    primaryFactory, secondaryFactory,
    onStateChangedCallback, onProgressCallback) {
  /**
   * @type {remoting.SignalStrategy}
   * @private
   */
  this.primary_ = primaryFactory(this.onPrimaryStateChanged_.bind(this));

  /**
   * @type {remoting.SignalStrategy}
   * @private
   */
  this.secondary_ = secondaryFactory(this.onSecondaryStateChanged_.bind(this));

  /**
   * @type {function(remoting.SignalStrategy.State)}
   * @private
   */
  this.onStateChangedCallback_ = onStateChangedCallback;

  /**
   * @type {function(remoting.FallbackSignalStrategy.Progress)}
   * @private
   */
  this.onProgressCallback_ = onProgressCallback;

  /**
   * @type {?function(Element):void}
   * @private
   */
  this.onIncomingStanzaCallback_ = null;

  /**
   * @type {number}
   * @private
   * @const
   */
  this.PRIMARY_CONNECT_TIMEOUT_MS_ = 10 * 1000;

  /**
   * @enum {string}
   * @private
   */
  this.State = {
    NOT_CONNECTED: 'not-connected',
    PRIMARY_PENDING: 'primary-pending',
    PRIMARY_SUCCEEDED: 'primary-succeeded',
    SECONDARY_PENDING: 'secondary-pending',
    SECONDARY_SUCCEEDED: 'secondary-succeeded',
    SECONDARY_FAILED: 'secondary-failed',
    CLOSED: 'closed'
  };

  /**
   * @type {string}
   * @private
   */
  this.state_ = this.State.NOT_CONNECTED;

  /**
   * @type {?remoting.SignalStrategy.State}
   * @private
   */
  this.externalState_ = null;

  /**
   * @type {string}
   * @private
   */
  this.server_ = '';

  /**
   * @type {string}
   * @private
   */
  this.username_ = '';

  /**
   * @type {string}
   * @private
   */
  this.authToken_ = '';

  /**
   * @type {number}
   * @private
   */
  this.primaryConnectTimerId_ = 0;
};

/**
 * @enum {string}
 */
remoting.FallbackSignalStrategy.Progress = {
  PRIMARY_SUCCEEDED: 'primary-succeeded',
  PRIMARY_FAILED: 'primary-failed',
  PRIMARY_TIMED_OUT: 'primary-timed-out',
  PRIMARY_SUCCEEDED_LATE: 'primary-succeeded-late',
  PRIMARY_FAILED_LATE: 'primary-failed-late',
  SECONDARY_SUCCEEDED: 'secondary-succeeded',
  SECONDARY_FAILED: 'secondary-failed'
};

remoting.FallbackSignalStrategy.prototype.dispose = function() {
  this.primary_.dispose();
  this.secondary_.dispose();
};

/**
 * @param {?function(Element):void} onIncomingStanzaCallback Callback to call on
 *     incoming messages.
 */
remoting.FallbackSignalStrategy.prototype.setIncomingStanzaCallback =
    function(onIncomingStanzaCallback) {
  this.onIncomingStanzaCallback_ = onIncomingStanzaCallback;
  if (this.state_ == this.State.PRIMARY_PENDING ||
      this.state_ == this.State.PRIMARY_SUCCEEDED) {
    this.primary_.setIncomingStanzaCallback(onIncomingStanzaCallback);
  } else if (this.state_ == this.State.SECONDARY_PENDING ||
             this.state_ == this.State.SECONDARY_SUCCEEDED) {
    this.secondary_.setIncomingStanzaCallback(onIncomingStanzaCallback);
  }
};

/**
 * @param {string} server
 * @param {string} username
 * @param {string} authToken
 */
remoting.FallbackSignalStrategy.prototype.connect =
    function(server, username, authToken) {
  base.debug.assert(this.state_ == this.State.NOT_CONNECTED);
  this.server_ = server;
  this.username_ = username;
  this.authToken_ = authToken;
  this.state_ = this.State.PRIMARY_PENDING;
  this.primary_.setIncomingStanzaCallback(this.onIncomingStanzaCallback_);
  this.primary_.connect(server, username, authToken);
  this.primaryConnectTimerId_ =
      window.setTimeout(this.onPrimaryTimeout_.bind(this),
                        this.PRIMARY_CONNECT_TIMEOUT_MS_);
};

/**
 * Sends a message. Can be called only in CONNECTED state.
 * @param {string} message
 */
remoting.FallbackSignalStrategy.prototype.sendMessage = function(message) {
  this.getConnectedSignalStrategy_().sendMessage(message);
};

/** @return {remoting.SignalStrategy.State} Current state */
remoting.FallbackSignalStrategy.prototype.getState = function() {
  return (this.externalState_ === null)
      ? remoting.SignalStrategy.State.NOT_CONNECTED
      : this.externalState_;
};

/** @return {remoting.Error} Error when in FAILED state. */
remoting.FallbackSignalStrategy.prototype.getError = function() {
  base.debug.assert(this.state_ == this.State.SECONDARY_FAILED);
  base.debug.assert(
      this.secondary_.getState() == remoting.SignalStrategy.State.FAILED);
  return this.secondary_.getError();
};

/** @return {string} Current JID when in CONNECTED state. */
remoting.FallbackSignalStrategy.prototype.getJid = function() {
  return this.getConnectedSignalStrategy_().getJid();
};

/**
 * @return {remoting.SignalStrategy} The active signal strategy, if the
 *     connection has succeeded.
 * @private
 */
remoting.FallbackSignalStrategy.prototype.getConnectedSignalStrategy_ =
    function() {
  if (this.state_ == this.State.PRIMARY_SUCCEEDED) {
    base.debug.assert(
        this.primary_.getState() == remoting.SignalStrategy.State.CONNECTED);
    return this.primary_;
  } else if (this.state_ == this.State.SECONDARY_SUCCEEDED) {
    base.debug.assert(
        this.secondary_.getState() == remoting.SignalStrategy.State.CONNECTED);
    return this.secondary_;
  } else {
    base.debug.assert(
        false,
        'getConnectedSignalStrategy called in unconnected state');
    return null;
  }
};

/**
 * @param {remoting.SignalStrategy.State} state
 * @private
 */
remoting.FallbackSignalStrategy.prototype.onPrimaryStateChanged_ =
    function(state) {
  switch (state) {
    case remoting.SignalStrategy.State.CONNECTED:
      if (this.state_ == this.State.PRIMARY_PENDING) {
        window.clearTimeout(this.primaryConnectTimerId_);
        this.onProgressCallback_(
            remoting.FallbackSignalStrategy.Progress.PRIMARY_SUCCEEDED);
        this.state_ = this.State.PRIMARY_SUCCEEDED;
      } else {
        this.onProgressCallback_(
            remoting.FallbackSignalStrategy.Progress.PRIMARY_SUCCEEDED_LATE);
      }
      break;

    case remoting.SignalStrategy.State.FAILED:
      if (this.state_ == this.State.PRIMARY_PENDING) {
        window.clearTimeout(this.primaryConnectTimerId_);
        this.onProgressCallback_(
            remoting.FallbackSignalStrategy.Progress.PRIMARY_FAILED);
        this.connectSecondary_();
      } else {
        this.onProgressCallback_(
            remoting.FallbackSignalStrategy.Progress.PRIMARY_FAILED_LATE);
      }
      return;  // Don't notify the external callback

    case remoting.SignalStrategy.State.CLOSED:
      this.state_ = this.State.CLOSED;
      break;
  }

  this.notifyExternalCallback_(state);
};

/**
 * @param {remoting.SignalStrategy.State} state
 * @private
 */
remoting.FallbackSignalStrategy.prototype.onSecondaryStateChanged_ =
    function(state) {
  switch (state) {
    case remoting.SignalStrategy.State.CONNECTED:
      this.onProgressCallback_(
          remoting.FallbackSignalStrategy.Progress.SECONDARY_SUCCEEDED);
      this.state_ = this.State.SECONDARY_SUCCEEDED;
      break;

    case remoting.SignalStrategy.State.FAILED:
      this.onProgressCallback_(
          remoting.FallbackSignalStrategy.Progress.SECONDARY_FAILED);
      this.state_ = this.State.SECONDARY_FAILED;
      break;

    case remoting.SignalStrategy.State.CLOSED:
      this.state_ = this.State.CLOSED;
      break;
  }

  this.notifyExternalCallback_(state);
};

/**
 * Notify the external callback of a change in state if it's consistent with
 * the allowed state transitions (ie, if it represents a later stage in the
 * connection process). Suppress state transitions that would violate this,
 * for example a CONNECTING -> NOT_CONNECTED transition when we switch from
 * the primary to the secondary signal strategy.
 *
 * @param {remoting.SignalStrategy.State} state
 * @private
 */
remoting.FallbackSignalStrategy.prototype.notifyExternalCallback_ =
    function(state) {
  if (this.externalState_ === null || state > this.externalState_) {
    this.externalState_ = state;
    this.onStateChangedCallback_(state);
  }
};

/**
 * @private
 */
remoting.FallbackSignalStrategy.prototype.connectSecondary_ = function() {
  base.debug.assert(this.state_ == this.State.PRIMARY_PENDING);
  base.debug.assert(this.server_ != '');
  base.debug.assert(this.username_ != '');
  base.debug.assert(this.authToken_ != '');

  this.state_ = this.State.SECONDARY_PENDING;
  this.primary_.setIncomingStanzaCallback(null);
  this.secondary_.setIncomingStanzaCallback(this.onIncomingStanzaCallback_);
  this.secondary_.connect(this.server_, this.username_, this.authToken_);
};

/**
 * @private
 */
remoting.FallbackSignalStrategy.prototype.onPrimaryTimeout_ = function() {
  this.onProgressCallback_(
      remoting.FallbackSignalStrategy.Progress.PRIMARY_TIMED_OUT);
  this.connectSecondary_();
};
