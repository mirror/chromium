// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import android.view.ViewGroup;

import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

/**
 * An action that can be performed by the Web Assistant in a tab.
 *
 * These are received from the WebAssistantService and performed by the WebAssistantController.
 */
abstract class WebAssistantAction {
    public static enum CheckResult {
        SHOW_ACTION,
        HIDE_ACTION,
    }

    public static interface ActionCheckCallback {
        public void onActionCheckedPage(WebAssistantAction action, CheckResult result);
    }

    /**
     * Returns null when it fails to parse the JSON argument.
     */
    public static WebAssistantAction parse(JSONObject json) {
        WebAssistantAction action = parseAction(json);
        if (action == null) return null;

        JSONObject conditions = json.optJSONObject("conditions");
        if (conditions != null) {
            action.mConditions = WebAssistantConditions.parse(conditions);
        }

        return action;
    }

    private static WebAssistantAction parseAction(JSONObject json) {
        JSONObject action = json.optJSONObject("message");
        if (action != null) {
            return WebAssistantMessageAction.parse(action);
        }
        action = json.optJSONObject("chips");
        if (action != null) {
            return WebAssistantChipsAction.parse(action);
        }
        return null;
    }

    private WebAssistantConditions mConditions;

    public void checkPage(DevToolsAgentHost devtools, ActionCheckCallback callback) {
        if (mConditions == null) {
            callback.onActionCheckedPage(this, CheckResult.SHOW_ACTION);
            return;
        }
        mConditions.checkConditions(devtools, (satisfied) -> {
            callback.onActionCheckedPage(
                    this, satisfied ? CheckResult.SHOW_ACTION : CheckResult.HIDE_ACTION);
        });
    }

    public abstract void buildView(ViewGroup bottomBar);
}
