// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ThemedWindowFrameBuilder;

/**
 * FirstRunActivity variant that fills the whole screen, but displays the content
 * in a dialog-like layout.
 */
public class TabbedModeFirstRunActivity extends FirstRunActivity {
    @Override
    protected View createContentView() {
        View contentView = super.createContentView();
        ThemedWindowFrameBuilder frameBuilder =
                new ThemedWindowFrameBuilder(this, R.style.FirstRunTheme);
        return frameBuilder.setDimBehind().setContentView(contentView).build();
    }
}
