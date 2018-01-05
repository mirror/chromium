// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.assist_actions;

import android.content.Context;
import android.widget.FrameLayout;

import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

/**
 * An action that can be performed by the Web Assistant in a tab.
 *
 * These are received from the WebAssistantService and performed by the WebAssistantController.
 */
public abstract class AssistAction {
    /** The result of checking whether an action is ready to be shown. */
    public static enum CheckResult {
        SHOW_ACTION,
        HIDE_ACTION,
    }

    /** The result of executing an action. */
    public static enum ActionResult {
        // The bottom bar was closed by the user.
        CLOSE,
        // An action was performed and succeeded.
        ACTION_OK,
        // An action was performed and failed.
        ACTION_FAILED,
    }

    /** Callback for checking whether a page is ready to trigger an action. */
    public static interface ActionCheckCallback {
        public void onActionCheckedPage(CheckResult result);
    }

    /**
     * Delegate interface for the creators of AssistActions.
     * Used to get access to the Devtools and to report the outcome of every action that gets
     * executed.
     */
    public static interface ActionDelegate {
        public DevToolsAgentHost getDevtools();
        public void onActionResult(ActionResult result);
    }

    /**
     * Returns null when it fails to parse the JSON argument.
     */
    public static AssistAction parse(JSONObject json) {
        AssistAction action = parseAction(json);
        if (action == null) return null;
        action.mConditions = Conditions.parse(json.optJSONObject("conditions"));
        return action;
    }

    private static AssistAction parseAction(JSONObject json) {
        if (json == null) return null;
        JSONObject message = json.optJSONObject("message");
        if (message != null) {
            return AssistActionMessage.parse(message);
        }
        JSONObject chips = json.optJSONObject("chips");
        if (chips != null) {
            return AssistActionChips.parse(chips);
        }
        return null;
    }

    private ActionDelegate mDelegate;
    private Conditions mConditions;

    public void setDelegate(ActionDelegate delegate) {
        mDelegate = delegate;
    }

    public void checkPage(DevToolsAgentHost devtools, ActionCheckCallback callback) {
        if (mConditions == null) {
            callback.onActionCheckedPage(CheckResult.SHOW_ACTION);
            return;
        }
        mConditions.checkConditions(devtools, (satisfied) -> {
            callback.onActionCheckedPage(
                    satisfied ? CheckResult.SHOW_ACTION : CheckResult.HIDE_ACTION);
        });
    }

    protected DevToolsAgentHost getDevtools() {
        return mDelegate == null ? null : mDelegate.getDevtools();
    }

    protected void onActionResult(ActionResult result) {
        if (mDelegate != null) {
            mDelegate.onActionResult(result);
        }
    }

    public abstract void buildView(Context context, FrameLayout bottomBar);
    public abstract void destroyView();
}
