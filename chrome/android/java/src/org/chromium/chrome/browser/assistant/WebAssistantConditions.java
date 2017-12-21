// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import org.json.JSONArray;
import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

/**
 * Checks if the current page fulfills a list of conditions.
 */
class WebAssistantConditions {
    public static interface ConditionsCheckCallback {
        public void onConditionsChecked(boolean satisfied);
    }

    public static WebAssistantConditions parse(JSONObject json) {
        WebAssistantConditions conditions = new WebAssistantConditions();

        JSONObject exists = json.optJSONObject("exists");
        if (exists != null) {
            JSONArray selector = exists.optJSONArray("selector");
            if (selector != null) {
                conditions.mExists = Selector.parse(selector);
            }
        }

        return conditions;
    }

    private Selector mExists;

    private WebAssistantConditions() {
        mExists = null;
    }

    public void checkConditions(DevToolsAgentHost devtools, ConditionsCheckCallback callback) {
        checkExists(devtools, (satisfied) -> {
            if (!satisfied) {
                callback.onConditionsChecked(false);
                return;
            }
            callback.onConditionsChecked(true);
        });
    }

    private void checkExists(DevToolsAgentHost devtools, ConditionsCheckCallback callback) {
        if (mExists == null) {
            callback.onConditionsChecked(true);
            return;
        }
        mExists.findMatches(devtools, (matches) -> {
            boolean satisfied = !matches.isEmpty();
            callback.onConditionsChecked(satisfied);
        });
    }
}
