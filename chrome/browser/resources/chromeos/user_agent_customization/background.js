// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Customization of User-Agent with usage of debugger API. */

/** Debugging protocol version. */
const PROTOCOL_VERSION = '1.2';
/** Pattern for CrOS platform in User-Agent string. */
const REGEX_PLATFORM_CROS = new RegExp('CrOS(?:(?!\\)).)*', 'i');
/** Windows platform string. */
const PLATFORM_WIN = 'Win x86_64';
/**
 * Sets if tabs with attached debugger.
 * @type {!Set<integer>}
 */
var attachedTabs = new Set();

// chrome.debugger.onEvent.addListener(onEvent);
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
      var tabId = sender.tab.id;
      var targetId = {tabId: tabId};
      if (!attachedTabs.has(tabId)) {
        attachedTabs.add(tabId);
        chrome.debugger.attach(
            targetId, PROTOCOL_VERSION, onAttach.bind(null, targetId));
      } else {
        onAttach(targetId);
      }
      break;
    default:
      console.error('Unsupported message:' + message);
  }
}

/**
 * Overrides User-Agent if debugger was successfully attached to the tab.
 * @param {DebuggeeId} targetId Id of debugger target tab.
 */
function onAttach(targetId) {
  if (chrome.runtime.lastError) {
    console.error('onAttach error:' + chrome.runtime.lastError.message);
    attachedTabs.delete(targetId.tabId);
    return;
  }
  chrome.debugger.sendCommand(
      targetId, 'Network.enable', onEnableSent.bind(null, null, targetId));
}

/**
 * Sends override command after netwoek was enabled.
 * @param {object=} result JSON object with result of Network.enable call.
 * @param {DebugeeId} targetId Id of debuger target tab.
 */
function onEnableSent(result, targetId) {
  console.log('onEnableSent');
  if (chrome.runtime.lastError) {
    console.error('onEnableSent error:' + chrome.runtime.lastError.message);
    return;
  }
  var userAgent =
      navigator.userAgent.replace(REGEX_PLATFORM_CROS, PLATFORM_WIN);
  var args = {userAgent: userAgent};
  chrome.debugger.sendCommand(
      targetId, 'Network.setUserAgentOverride', args,
      onOverrideSent.bind(null, null, targetId));
}

/**
 * Detaches debugger after override command was send.
 * @param {object=} result JSON object with result of
 *     Network.setUserAgentOverride call.
 * @param {DebugeeId} targetId Id of debuger target tab.
 */
function onOverrideSent(result, targetId) {
  console.log('onSendCommand');
  if (chrome.runtime.lastError) {
    console.error('onSendCommand error:' + chrome.runtime.lastError.message);
  }
}

/**
 * Handles detaching debugger. Removes the tab from the set when debugger was
 * successfully detached.
 * @param {DebuggeeId} targetId Id of debugger target tab.
 */
function onDetach(targetId) {
  if (chrome.runtime.lastError) {
    console.log('onDetach error:' + chrome.runtime.lastError.message);
  } else {
    attachedTabs.delete(targetId.tabId);
  }
}

/** Detaches debugger from tabs. */
function onSuspend() {
  attachedTabs.forEach(function(element) {
    var targetId = {tabId: tabId};
    chrome.debugger.detach(targetId, onDetach.bind(null, targetId));
  }, this);
}
