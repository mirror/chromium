// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Temporary shim to allow Select-to-Speak to use Chromevox utils, but
 * providing fake objects where needed.
 */

// For some reason, the compiler cannot find StateType.visited, although
// it can find StateType.VISITED. Provide a shim.
chrome.automation.StateType.visited = 'visited';

// Shim out cvox.
var cvox = {};
/**
 * @typedef {{x: number, y: number}}
 */
cvox.Point;
