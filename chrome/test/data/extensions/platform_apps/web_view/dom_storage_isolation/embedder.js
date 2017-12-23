// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function initDOMStorage(name) {
  window.localStorage.setItem('foo', 'local-' + name);
  window.sessionStorage.setItem('baz', 'session-' + name);
}

function getLocalStorage() {
  return window.localStorage.getItem('foo') || 'badval';
}

function getSessionStorage() {
  return window.sessionStorage.getItem('baz') || 'badval';
}

function setWindowName(name) {
  window.name = name;
}

function findWindowByName(name) {
  var w = window.open('', name);
  var found = (w.location.href != "about:blank");
  if (!found)
    w.close();
  return found;
}

function initialize() {
  var messageHandler = new Messaging.Handler();
  messageHandler.addHandler(INIT_DOM_STORAGE, function(message, portFrom) {
    initDOMStorage(message.pageName);
    messageHandler.sendMessage({title: INIT_DOM_STORAGE_COMPLETE}, portFrom);
  });
  messageHandler.addHandler(GET_DOM_STORAGE_INFO, function(message, portFrom) {
    messageHandler.sendMessage(new Messaging.Message(
        GET_DOM_STORAGE_INFO_COMPLETE, {
          local: getLocalStorage(),
          session: getSessionStorage()
        }), portFrom);
  });
  messageHandler.addHandler(SET_WINDOW_NAME, function(message, portFrom) {
    setWindowName(message.windowName);
    messageHandler.sendMessage({title: SET_WINDOW_NAME_COMPLETE}, portFrom);
  });
  messageHandler.addHandler(FIND_WINDOW_BY_NAME, function(message, portFrom) {
    var found = findWindowByName(message.windowName);
    messageHandler.sendMessage(
        {title: FIND_WINDOW_BY_NAME_COMPLETE, found: found},
        portFrom);
  });
}
window.onload = initialize;
