// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.page_actions;

import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

/** Base class for actions that mutate the current page. */
public abstract class PageAction {
    /** Interface to receive the result of the execution of an action on the page. */
    public static interface ActionResultCallback { public void onActionResult(boolean succeeded); }

    public static PageAction parse(JSONObject json) {
        if (json == null) return null;
        JSONObject click = json.optJSONObject("click");
        if (click != null) {
            return PageActionClick.parse(click);
        }
        JSONObject input = json.optJSONObject("input");
        if (input != null) {
            return PageActionInput.parse(input);
        }
        return null;
    }

    private boolean mInProgress;

    protected PageAction() {
        mInProgress = false;
    }

    public final void doPageAction(DevToolsAgentHost devtools, ActionResultCallback callback) {
        if (mInProgress || devtools == null) return;
        mInProgress = true;
        doAction(devtools, (succeeded) -> {
            mInProgress = false;
            callback.onActionResult(succeeded);
        });
    }

    public void cancel() {}

    protected abstract void doAction(DevToolsAgentHost devtools, ActionResultCallback callback);
}
