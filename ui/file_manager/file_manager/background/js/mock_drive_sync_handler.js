// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

/**
 * Mock of DriveSyncHandler.
 * @constructor
 * @struct
 */
function MockDriveSyncHandler() {
  this.listeners_ = Object.create(null);
}

/**
 * @param {string} type The name of the event.
 * @param {EventListenerType} handler The handler for the event.
 */
MockDriveSyncHandler.prototype.removeEventListener = function(type, handler) {
  delete this.listeners_[type];
};

/**
 * @param {string} type The name of the event.
 * @param {EventListenerType} handler The handler for the event.
 */
MockDriveSyncHandler.prototype.addEventListener = function(type, handler) {
  this.listeners_[type] = [handler];
  window.setTimeout(function(type, handler) {
    this.listeners_[type][0].call(handler, null);
  }.bind(this, type, handler), 500);  // 0.5 second
};
