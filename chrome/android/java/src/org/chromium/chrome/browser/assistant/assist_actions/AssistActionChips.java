// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.assist_actions;

import android.content.Context;
import android.widget.FrameLayout;

import org.json.JSONObject;

class AssistActionChips extends AssistAction {
    public static AssistActionChips parse(JSONObject json) {
        // TODO
        return new AssistActionChips();
    }

    @Override
    public void buildView(Context context, FrameLayout bottomBar) {
        // TODO
    }

    @Override
    public void destroyView() {
        // TODO
    }
}
