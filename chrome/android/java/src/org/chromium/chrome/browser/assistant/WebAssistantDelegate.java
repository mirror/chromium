// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import android.content.res.Resources;

import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.fullscreen.ChromeFullscreenManager;
import org.chromium.chrome.browser.toolbar.ToolbarManager;

/** Interface to embed the WebAssistant in an Activity. */
public interface WebAssistantDelegate {
    public Resources getResources();
    public ToolbarManager getToolbarManager();
    public ChromeActivity getActivity();
    public ChromeFullscreenManager getFullscreenManager();
}
