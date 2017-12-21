// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import org.json.JSONArray;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

import java.util.ArrayList;

/**
 * Helper to locate DOM nodes in a webpage.
 */
class Selector {
    public static interface MatchesCallback {
        public void onSelectorMatches(ArrayList<String> nodeIds);
    }

    public static Selector parse(JSONArray selector) {
        ArrayList<String> selectors = new ArrayList<String>(selector.length());
        for (int i = 0; i < selector.length(); i++) {
            String s = selector.optString(i);
            if (s != null && !"".equals(s)) {
                selectors.add(s);
            }
        }
        return new Selector(selectors);
    }

    private ArrayList<String> mSelectors;

    private Selector(ArrayList<String> selectors) {
        mSelectors = selectors;
    }

    public void findMatches(DevToolsAgentHost devtools, MatchesCallback callback) {
        // TODO
    }
}
