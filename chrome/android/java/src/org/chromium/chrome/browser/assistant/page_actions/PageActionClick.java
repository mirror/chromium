// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.page_actions;

import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

/** A page action that issues clicks on the page. */
class PageActionClick extends PageAction {
    public static PageActionClick parse(JSONObject json) {
        if (json == null) return null;
        Selector selector = Selector.parse(json, "selector");
        if (selector == null) return null;
        return new PageActionClick(selector);
    }

    private final Selector mSelector;

    private PageActionClick(Selector selector) {
        mSelector = selector;
    }

    @Override
    public void doAction(DevToolsAgentHost devtools, ActionResultCallback callback) {
        mSelector.click(devtools, (ok) -> callback.onActionResult(ok));
    }
}
