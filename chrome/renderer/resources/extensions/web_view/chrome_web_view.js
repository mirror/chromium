// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements chrome-specific <webview> API.
// See web_view_api_methods.js for details.

var ChromeWebView = require('chromeWebViewInternal').ChromeWebView;
var ChromeWebViewSchema =
    requireNative('schema_registry').GetSchema('chromeWebViewInternal');
var CreateEvent = require('guestViewEvents').CreateEvent;
var EventBindings = require('event_bindings');
var idGeneratorNatives = requireNative('id_generator');
var Utils = require('utils');
var WebViewEvents = require('webViewEvents').WebViewEvents;
var WebViewImpl = require('webView').WebViewImpl;

var CHROME_WEB_VIEW_EVENTS = {
  'contextmenushown': {
    evt: CreateEvent('chromeWebViewInternal.contextmenu'),
    cancelable: true,
    fields: ['items'],
    handler: 'handleContextMenu'
  }
};

// This is the only "webViewInternal.onClicked" named event for this renderer.
//
// Since we need an event per <webview>, we define events with suffix
// (subEventName) in each of the <webview>. Behind the scenes, this event is
// registered as a ContextMenusEvent, with filter set to the webview's
// |viewInstanceId|. Any time a ContextMenusEvent is dispatched, we re-dispatch
// it to the subEvent's listeners. This way
// <webview>.contextMenus.onClicked behave as a regular chrome Event type.
var ContextMenusEvent = CreateEvent('chromeWebViewInternal.onClicked');

// -----------------------------------------------------------------------------
// ContextMenusOnClickedEvent object.

// This event is exposed as <webview>.contextMenus.onClicked.
function ContextMenusOnClickedEvent(opt_eventName,
                                    opt_argSchemas,
                                    opt_eventOptions,
                                    opt_webViewInstanceId) {
  var subEventName = GetUniqueSubEventName(opt_eventName);
  EventBindings.Event.call(this,
                           subEventName,
                           opt_argSchemas,
                           opt_eventOptions,
                           opt_webViewInstanceId);

  // TODO(lazyboy): When do we dispose this listener?
  ContextMenusEvent.addListener(function() {
    // Re-dispatch to subEvent's listeners.
    $Function.apply(this.dispatch, this, $Array.slice(arguments));
  }.bind(this), {instanceId: opt_webViewInstanceId || 0});
}

ContextMenusOnClickedEvent.prototype.__proto__ = EventBindings.Event.prototype;

// -----------------------------------------------------------------------------
// WebViewContextMenusImpl object.

// An instance of this class is exposed as <webview>.contextMenus.
function WebViewContextMenusImpl(viewInstanceId) {
  this.viewInstanceId_ = viewInstanceId;
}

WebViewContextMenusImpl.prototype.create = function() {
  var args = $Array.concat([this.viewInstanceId_], $Array.slice(arguments));
  return $Function.apply(ChromeWebView.contextMenusCreate, null, args);
};

WebViewContextMenusImpl.prototype.remove = function() {
  var args = $Array.concat([this.viewInstanceId_], $Array.slice(arguments));
  return $Function.apply(ChromeWebView.contextMenusRemove, null, args);
};

WebViewContextMenusImpl.prototype.removeAll = function() {
  var args = $Array.concat([this.viewInstanceId_], $Array.slice(arguments));
  return $Function.apply(ChromeWebView.contextMenusRemoveAll, null, args);
};

WebViewContextMenusImpl.prototype.update = function() {
  var args = $Array.concat([this.viewInstanceId_], $Array.slice(arguments));
  return $Function.apply(ChromeWebView.contextMenusUpdate, null, args);
};

var WebViewContextMenus = Utils.expose(
    'WebViewContextMenus', WebViewContextMenusImpl,
    { functions: ['create', 'remove', 'removeAll', 'update'] });

// -----------------------------------------------------------------------------

WebViewImpl.prototype.maybeSetupContextMenus = function() {
  var createContextMenus = function() {
    return function() {
      if (this.contextMenus_) {
        return this.contextMenus_;
      }

      this.contextMenus_ = new WebViewContextMenus(this.viewInstanceId);

      // Define 'onClicked' event property on |this.contextMenus_|.
      var getOnClickedEvent = function() {
        return function() {
          if (!this.contextMenusOnClickedEvent_) {
            var eventName = 'chromeWebViewInternal.onClicked';
            // TODO(lazyboy): Find event by name instead of events[0].
            var eventSchema = ChromeWebViewSchema.events[0];
            var eventOptions = {supportsListeners: true};
            var onClickedEvent = new ContextMenusOnClickedEvent(
                eventName, eventSchema, eventOptions, this.viewInstanceId);
            this.contextMenusOnClickedEvent_ = onClickedEvent;
            return onClickedEvent;
          }
          return this.contextMenusOnClickedEvent_;
        }.bind(this);
      }.bind(this);
      Object.defineProperty(
          this.contextMenus_,
          'onClicked',
          {get: getOnClickedEvent(), enumerable: true});

      return this.contextMenus_;
    }.bind(this);
  }.bind(this);

  // Expose <webview>.contextMenus object.
  Object.defineProperty(
      this.element,
      'contextMenus',
      {
        get: createContextMenus(),
        enumerable: true
      });
};

WebViewEvents.prototype.handleContextMenu = function(event, eventName) {
  var webViewEvent = this.makeDomEvent(event, eventName);
  var requestId = event.requestId;
  // Construct the event.menu object.
  var actionTaken = false;
  var validateCall = function() {
    var ERROR_MSG_CONTEXT_MENU_ACTION_ALREADY_TAKEN = '<webview>: ' +
        'An action has already been taken for this "contextmenu" event.';

    if (actionTaken) {
      throw new Error(ERROR_MSG_CONTEXT_MENU_ACTION_ALREADY_TAKEN);
    }
    actionTaken = true;
  };
  var menu = {
    show: function(items) {
      validateCall();
      // TODO(lazyboy): WebViewShowContextFunction doesn't do anything useful
      // with |items|, implement.
      ChromeWebView.showContextMenu(this.view.guest.getId(), requestId, items);
    }.bind(this)
  };
  webViewEvent.menu = menu;
  var element = this.view.element;
  var defaultPrevented = !element.dispatchEvent(webViewEvent);
  if (actionTaken) {
    return;
  }
  if (!defaultPrevented) {
    actionTaken = true;
    // The default action is equivalent to just showing the context menu as is.
    ChromeWebView.showContextMenu(
        this.view.guest.getId(), requestId, undefined);

    // TODO(lazyboy): Figure out a way to show warning message only when
    // listeners are registered for this event.
  } //  else we will ignore showing the context menu completely.
};

function GetUniqueSubEventName(eventName) {
  return eventName + '/' + idGeneratorNatives.GetNextId();
}

// Exposes |CHROME_WEB_VIEW_EVENTS| when the ChromeWebView API is available.
(function() {
  for (var eventName in CHROME_WEB_VIEW_EVENTS) {
    WebViewEvents.EVENTS[eventName] = CHROME_WEB_VIEW_EVENTS[eventName];
  }
})();
