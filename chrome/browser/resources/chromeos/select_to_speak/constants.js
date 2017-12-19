// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO: Refactor to combine in a single file with
// chrome/browser/resources/chromevox/cvox2/background/constants.js

/**
 * @fileoverview Constants used throughout ChromeVox.
 */

var AutomationNode = chrome.automation.AutomationNode;
var Restriction = chrome.automation.Restriction;
var Role = chrome.automation.RoleType;
var State = chrome.automation.StateType;
var RoleType = chrome.automation.RoleType;

var constants = function() {};

/**
 * Possible directions to perform tree traversals.
 * @enum {string}
 */
constants.Dir = {
  /** Search from left to right. */
  FORWARD: 'forward',

  /** Search from right to left. */
  BACKWARD: 'backward'
};

/**
 * If a node contains more characters than this, it should not be visited during
 * object navigation.
 *
 * This number was taken from group_util.js and is an approximate average of
 * paragraph length. It's purpose is to prevent overloading tts.
 * @type {number}
 * @const
 */
constants.OBJECT_MAX_CHARCOUNT = 1500;

/**
 * Modifier values used by Chrome.
 * See ui/events/event_constants.h
 */
constants.ModifierFlag = {
  SHIFT: 2,
  CONTROL: 4,
  ALT: 8,
  SEARCH: 16
};
