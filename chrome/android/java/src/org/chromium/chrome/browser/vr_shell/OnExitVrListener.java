// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

/**
 * Callback to be invoked when VR was exited.
 */
public interface OnExitVrListener {
    /**
     * Called when VR was exited. May not be called if an exit VR request was denied (e.g. user
     * chose to not exit VR).
     */
    void onExited();
}