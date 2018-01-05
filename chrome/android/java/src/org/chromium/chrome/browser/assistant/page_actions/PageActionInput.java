// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.page_actions;

import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

/** A page action that enters input data into the page. */
class PageActionInput extends PageAction {
    public static PageActionInput parse(JSONObject json) {
        if (json == null) return null;
        Selector selector = Selector.parse(json, "selector");
        String value = json.optString("value");
        if (selector == null || value == null) return null;
        return new PageActionInput(selector, value);
    }

    private final Selector mSelector;
    private final String mValue;

    private PageActionInput(Selector selector, String value) {
        mSelector = selector;
        mValue = value;
    }

    @Override
    public void doAction(DevToolsAgentHost devtools, ActionResultCallback callback) {
        mSelector.setValue(devtools, mValue, (ok) -> callback.onActionResult(ok));
    }
}
