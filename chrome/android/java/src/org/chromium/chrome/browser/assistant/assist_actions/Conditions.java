// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.assist_actions;

import org.json.JSONObject;

import org.chromium.chrome.browser.assistant.page_actions.Selector;
import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

/**
 * Checks if the current page fulfills a list of conditions.
 */
class Conditions {
    public static interface ConditionsCheckCallback {
        public void onConditionsChecked(boolean satisfied);
    }

    public static Conditions parse(JSONObject json) {
        if (json == null) return null;
        Conditions conditions = new Conditions();
        conditions.mExists = Selector.parse(json, "exists");
        conditions.mIsVisible = Selector.parse(json, "is_visible");
        return conditions;
    }

    private Selector mExists;
    private Selector mIsVisible;

    private Conditions() {
        mExists = null;
        mIsVisible = null;
    }

    public void checkConditions(DevToolsAgentHost devtools, ConditionsCheckCallback callback) {
        checkExists(devtools, (check1) -> {
            if (!check1) {
                callback.onConditionsChecked(false);
                return;
            }
            checkIsVisible(devtools, (check2) -> {
                if (!check2) {
                    callback.onConditionsChecked(false);
                    return;
                }
                callback.onConditionsChecked(true);
            });
        });
    }

    private void checkExists(DevToolsAgentHost devtools, ConditionsCheckCallback callback) {
        if (mExists == null) {
            callback.onConditionsChecked(true);
            return;
        }
        mExists.findMatches(devtools, (matches) -> {
            boolean satisfied = matches.length > 0;
            callback.onConditionsChecked(satisfied);
        });
    }

    private void checkIsVisible(DevToolsAgentHost devtools, ConditionsCheckCallback callback) {
        if (mIsVisible == null) {
            callback.onConditionsChecked(true);
            return;
        }
        mIsVisible.isVisible(devtools, (visible) -> callback.onConditionsChecked(visible));
    }
}
