// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.page_actions;

import org.json.JSONArray;
import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

import java.util.ArrayList;

/** A list of page actions, executed serially. */
public class PageActionList extends PageAction {
    public static PageActionList parse(JSONObject json) {
        if (json == null) return null;
        JSONArray list = json.optJSONArray("action");
        if (list == null || list.length() == 0) return null;
        ArrayList<PageAction> actions = new ArrayList<PageAction>();
        for (int i = 0; i < list.length(); i++) {
            PageAction action = PageAction.parse(list.optJSONObject(i));
            if (action == null) return null;
            actions.add(action);
        }
        return new PageActionList(actions);
    }

    private final ArrayList<PageAction> mActions;

    private PageActionList(ArrayList<PageAction> actions) {
        mActions = actions;
    }

    @Override
    public void doAction(DevToolsAgentHost devtools, ActionResultCallback callback) {
        doAction(devtools, 0, callback);
    }

    private void doAction(DevToolsAgentHost devtools, int index, ActionResultCallback callback) {
        if (index < 0 || index >= mActions.size()) {
            callback.onActionResult(true);
            return;
        }
        mActions.get(index).doAction(devtools, (succeeded) -> {
            if (succeeded) {
                doAction(devtools, index + 1, callback);
            } else {
                callback.onActionResult(false);
            }
        });
    }
}
