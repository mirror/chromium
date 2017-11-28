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
  this.listeners_ = {};
}

/**
 * @param {string} type The name of the event.
 * @param {EventListenerType} handler The handler for the event.
 */
MockDriveSyncHandler.prototype.removeEventListener = function(type, handler) {
  if (type in this.listeners_) {
    var handlers = this.listeners_[type];
    var index = handlers.indexOf(handler);
    if (index > -1) {
      if (this.listeners_[type].length > 1) {
        handlers.splice(index, 1);
      } else {
        delete this.listeners_[type];
      }
    }
  }
};

/**
 * @param {string} type The name of the event.
 * @param {EventListenerType} handler The handler for the event.
 */
MockDriveSyncHandler.prototype.addEventListener = function(type, handler) {
  if (type in this.listeners_) {
    var handlers = this.listeners_[type];
    handlers.push(handler);
  } else {
    this.listeners_[type] = [handler];
  }
};
