// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Customization of User-Agent with usage of debugger API. */

/** Debugging protocol version. */
const PROTOCOL_VERSION = '1.2';
/** Pattern for CrOS platform in User-Agent string. */
const REGEX_PLATFORM_CROS = new RegExp('X11(?:(?!\\)).)*', 'i');
/** Windows platform string. */
const PLATFORM_WIN = 'Windows NT 6.1';
/**
 * Sets if tabs with attached debugger.
 * @type {!Set<integer>}
 */
var attachedTabs = new Set();

chrome.debugger.onDetach.addListener(onDetach);
chrome.runtime.onMessage.addListener(onMessage);
chrome.runtime.onSuspend.addListener(onSuspend);

/**
 * Processes message. Supported messages:
 *   - START attaches debugger and starts overriding User-Agent
 * @param {any=} message Message sent by calling script.
 * @param {MessageSender} sender Sender of the message.
 * @param {function} sendResponse Function to call when you have the response.
 */
function onMessage(message, sender, sendResponse) {
  switch (message) {
    case 'START':
      if (attachedTabs.has(sender.tab.id)) {
        sendResponse({shouldStop: false});
        return;
      }
      sendResponse({shouldStop: true});
      attachedTabs.add(sender.tab.id);
      var target = {tabId: sender.tab.id};
      chrome.debugger.attach(
          target, PROTOCOL_VERSION, onAttach.bind(null, target));
      break;
    default:
      console.error('Unsupported message:' + message);
  }
}

/**
 * Overrides User-Agent if debugger was successfully attached to the tab.
 * @param {DebuggeeId} target Debugger target tab.
 */
function onAttach(target) {
  if (chrome.runtime.lastError) {
    console.error('onAttach error:' + chrome.runtime.lastError.message);
    attachedTabs.delete(target.tabId);
    chrome.tabs.reload(target.tabId);
    return;
  }
  chrome.debugger.sendCommand(
      target, 'Network.enable', onEnableSent.bind(null, target));
}

/**
 * Sends override command after netwoek was enabled.
 * @param {DebugeeId} target Debugger target tab.
 * @param {object=} result JSON object with result of Network.enable call.
 */
function onEnableSent(target, result) {
  console.log('onEnableSent');
  if (chrome.runtime.lastError) {
    console.error('onEnableSent error:' + chrome.runtime.lastError.message);
  }
  var userAgent =
      navigator.userAgent.replace(REGEX_PLATFORM_CROS, PLATFORM_WIN);
  var args = {userAgent: userAgent};
  chrome.debugger.sendCommand(
      target, 'Network.setUserAgentOverride', args,
      onOverrideSent.bind(null, target));
}

/**
 * Detaches debugger after override command was send.
 * @param {DebugeeId} target Debuger target tab.
 * @param {object=} result JSON object with result of
 *     Network.setUserAgentOverride call.
 */
function onOverrideSent(target, result) {
  if (chrome.runtime.lastError) {
    console.error('onSendCommand error:' + chrome.runtime.lastError.message);
  }
  chrome.tabs.reload(target.tabId);
}

/**
 * Handles detaching debugger. Removes the tab from the set when debugger was
 * successfully detached.
 * @param {DebuggeeId} target Debugger target tab.
 */
function onDetach(target) {
  if (chrome.runtime.lastError) {
    console.log('onDetach error:' + chrome.runtime.lastError.message);
  } else {
    attachedTabs.delete(target.tabId);
  }
}

/** Detaches debugger from the tabs. */
function onSuspend() {
  console.log('onSuspend');
  attachedTabs.forEach(function(tabId) {
    var target = {tabId: tabId};
    console.log('onSuspend: detach ' + tabId);
    chrome.debugger.detach(target, onDetach.bind(null, target));
  }, this);
}
