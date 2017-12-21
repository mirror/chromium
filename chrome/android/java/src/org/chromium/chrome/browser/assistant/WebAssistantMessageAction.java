// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import android.view.ViewGroup;

import org.json.JSONObject;

class WebAssistantMessageAction extends WebAssistantAction {
    public static WebAssistantMessageAction parse(JSONObject json) {
        String message = json.optString("message");
        if (message == null || "".equals(message)) return null;
        // TODO: parse "on_tap"
        return new WebAssistantMessageAction(message);
    }

    private final String mMessage;

    private WebAssistantMessageAction(String message) {
        mMessage = message;
    }

    @Override
    public void buildView(ViewGroup bottomBar) {
        // TODO
    }
}
