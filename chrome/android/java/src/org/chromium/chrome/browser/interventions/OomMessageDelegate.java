// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.interventions;

import android.support.annotation.DrawableRes;

import org.chromium.chrome.R;

/**
 * Defines the appearance and callbacks for
 * {@link org.chromium.chrome.browser.infobar.FramebustBlockInfoBar}.
 */
public class OomMessageDelegate {
    /** The message describing the intervention. */
    public String getMessage() {
        return "This page uses too much memory, so Chrome paused it";
    }

    /** The icon to show in this infobar. */
    @DrawableRes
    public int getIconResourceId() {
        return R.drawable.infobar_chrome;
    }

    // TODO: only return resource ID?
    public String getEscapeHatchText() {
        return "Show original";
    }

    /** Callback called when the user wants to bypass the intervention. */
    public void escapeHatch() {}
}
