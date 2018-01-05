// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.page_actions;

import org.json.JSONArray;
import org.json.JSONObject;

import org.chromium.chrome.browser.devtools.DevToolsAgentHost;

import java.util.ArrayList;

/**
 * Helper to locate DOM nodes in a webpage.
 */
public class Selector {
    private static final int NO_MATCH = -1;
    private static final int[] NO_MATCHES = new int[0];

    private static final String GET_BOUNDING_RECT_JS = ""
            + "function() {\n"
            + "  const r = this.getBoundingClientRect();\n"
            + "  return {\n"
            + "    windowWidth: window.innerWidth,\n"
            + "    windowHeight: window.innerHeight,\n"
            + "    top: r.top,\n"
            + "    left: r.left,\n"
            + "    bottom: r.bottom,\n"
            + "    right: r.right,\n"
            + "  };\n"
            + "}\n";

    private static final String SET_VALUE_JS = ""
            + "function() {\n"
            + "  const param = %s;\n"
            + "  this.value = param.value;\n"
            + "  return {\n"
            + "    succeeded: true,\n"
            + "  };\n"
            + "}\n";

    private static final String CLICK_JS = ""
            + "function() {\n"
            + "  this.click();\n"
            + "  return {\n"
            + "    succeeded: true,\n"
            + "  };\n"
            + "}\n";

    /** Callback to receive a boolean result. */
    public static interface BooleanCallback { public void onBooleanResult(boolean result); }

    /** Callback to receive a devtools remote Object ID. */
    public static interface ObjectIdCallback {
        // Will be null on errors.
        public void onObjectId(String objectId);
    }

    /** Callback to receive a JSON object. */
    public static interface JSONCallback {
        // Will be null on errors.
        public void onJSONResult(JSONObject result);
    }

    /** Callback to receive a Node ID. */
    public static interface MatchCallback {
        // "nodeId" will be negative in case of failure.
        public void onNodeMatch(int nodeId);
    }

    /** Callback to receive a list of Node IDs. */
    public static interface MatchesCallback {
        // The array is never null but may have 0 elements.
        public void onNodeMatches(int[] nodeIds);
    }

    public static Selector parse(JSONArray selector) {
        if (selector == null) return null;
        ArrayList<String> selectors = new ArrayList<String>(selector.length());
        for (int i = 0; i < selector.length(); i++) {
            String s = selector.optString(i);
            if (s != null && !"".equals(s)) {
                selectors.add(s);
            }
        }
        return new Selector(selectors);
    }

    public static Selector parse(JSONObject json, String key) {
        if (json == null || key == null) return null;
        JSONObject field = json.optJSONObject(key);
        if (field == null) return null;
        return parse(field.optJSONArray("selector"));
    }

    private ArrayList<String> mSelectors;

    private Selector(ArrayList<String> selectors) {
        mSelectors = selectors;
    }

    public void findMatches(DevToolsAgentHost devtools, MatchesCallback callback) {
        // This method should only check whether the nodes exist. It shouldn't check whether they're
        // visible or not, and it shouldn't affect the page at all.
        // TODO(joaodasilva): add support for iframes.
        if (mSelectors.isEmpty()) {
            fail(callback);
            return;
        }
        getRootNodeId(devtools, (rootId) -> {
            if (rootId < 0) {
                fail(callback);
                return;
            }
            recurseFindMatches(devtools, rootId, 0, callback);
        });
    }

    public void isVisible(DevToolsAgentHost devtools, BooleanCallback callback) {
        findMatches(devtools, (matches) -> { checkIsVisible(devtools, matches, 0, callback); });
    }

    public void setValue(DevToolsAgentHost devtools, String value, BooleanCallback callback) {
        findMatches(devtools, (matches) -> { setValue(devtools, matches, 0, value, callback); });
    }

    public void click(DevToolsAgentHost devtools, BooleanCallback callback) {
        findMatches(devtools, (matches) -> {
            if (matches.length != 1) {
                callback.onBooleanResult(false);
                return;
            }
            callFunctionOnNodeId(devtools, matches[0], CLICK_JS, (result) -> {
                boolean succeeded = result != null && result.optBoolean("succeeded", false);
                callback.onBooleanResult(succeeded);
            });
        });
    }

    private void setValue(DevToolsAgentHost devtools, int[] nodeIds, int index, String value,
            BooleanCallback callback) {
        if (nodeIds == null || index < 0 || index >= nodeIds.length) {
            callback.onBooleanResult(true);
            return;
        }
        setValue(devtools, nodeIds[index], value, (succeeded) -> {
            if (!succeeded) {
                callback.onBooleanResult(false);
                return;
            }
            setValue(devtools, nodeIds, index + 1, value, callback);
        });
    }

    private void setValue(
            DevToolsAgentHost devtools, int nodeId, String value, BooleanCallback callback) {
        String function = makeSetValueFunction(value);
        if (function == null) {
            callback.onBooleanResult(false);
            return;
        }
        callFunctionOnNodeId(devtools, nodeId, function, (result) -> {
            boolean succeeded = result != null && result.optBoolean("succeeded", false);
            callback.onBooleanResult(succeeded);
        });
    }

    private void checkIsVisible(
            DevToolsAgentHost devtools, int[] nodeIds, int index, BooleanCallback callback) {
        if (nodeIds == null || index < 0 || index >= nodeIds.length) {
            callback.onBooleanResult(false);
            return;
        }
        checkIsVisible(devtools, nodeIds[index], (visible) -> {
            if (visible) {
                callback.onBooleanResult(true);
                return;
            }
            checkIsVisible(devtools, nodeIds, index + 1, callback);
        });
    }

    private void checkIsVisible(DevToolsAgentHost devtools, int nodeId, BooleanCallback callback) {
        callFunctionOnNodeId(devtools, nodeId, GET_BOUNDING_RECT_JS, (rect) -> {
            if (rect == null) {
                callback.onBooleanResult(false);
                return;
            }
            int top = rect.optInt("top", 0);
            int left = rect.optInt("left", 0);
            int right = rect.optInt("right", 0);
            int width = rect.optInt("windowWidth", -1);
            int height = rect.optInt("windowHeight", -1);
            int centerx = left + (right - left) / 2;
            // Consider the element to be visible if:
            //   - its Y (top) starts at least at 10% of the viewport height;
            //   - its Y (top) starts at most at 70% of the viewport height;
            //   - its X center is within the viewport width.
            boolean visible =
                    top >= height * 0.1 && top <= height * 0.7 && centerx > 0 && centerx < width;
            callback.onBooleanResult(visible);
        });
    }

    private void callFunctionOnNodeId(
            DevToolsAgentHost devtools, int nodeId, String function, JSONCallback callback) {
        resolveNodeIdToObjectId(devtools, nodeId, (objectId) -> {
            if (objectId == null) {
                callback.onJSONResult(null);
                return;
            }
            callFunctionOnObjectId(devtools, objectId, function, callback);
        });
    }

    private void callFunctionOnObjectId(
            DevToolsAgentHost devtools, String objectId, String function, JSONCallback callback) {
        String params = null;
        try {
            JSONObject json = new JSONObject();
            json.put("objectId", objectId);
            json.put("functionDeclaration", function);
            json.put("returnByValue", true);
            params = json.toString();
        } catch (Exception e) {
            callback.onJSONResult(null);
            return;
        }
        devtools.sendCommand("Runtime.callFunctionOn", params, (result, error) -> {
            JSONObject json = parseOrNull(result);
            if (json != null) {
                json = json.optJSONObject("result");
                if (json != null) {
                    json = json.optJSONObject("value");
                }
            }
            callback.onJSONResult(json);
        });
    }

    private void resolveNodeIdToObjectId(
            DevToolsAgentHost devtools, int nodeId, ObjectIdCallback callback) {
        String params = "{ \"nodeId\": " + nodeId + " }";
        devtools.sendCommand("DOM.resolveNode", params, (result, error) -> {
            String objectId = null;
            JSONObject json = parseOrNull(result);
            if (json != null) {
                JSONObject object = json.optJSONObject("object");
                if (object != null) {
                    objectId = object.optString("objectId", null);
                }
            }
            callback.onObjectId(objectId);
        });
    }

    private void recurseFindMatches(
            DevToolsAgentHost devtools, int rootId, int index, MatchesCallback callback) {
        if (index >= mSelectors.size()) {
            fail(callback);
            return;
        }
        String selector = mSelectors.get(index);
        querySelectorAll(devtools, rootId, selector, (matches) -> {
            if (matches == null || matches.length == 0) {
                fail(callback);
                return;
            }
            if (index == mSelectors.size() - 1) {
                // Last selector, this is the final result.
                callback.onNodeMatches(matches);
                return;
            }
            // Must have exactly one match and must be an iframe.
            // TODO(joaodasilva): implement: verify that it's an iframe, then get its inner document
            // nodeId and recurse into
            // recurseFindMatches(devtools, frameRootId, index + 1, callback).
            fail(callback);
        });
    }

    private void getRootNodeId(DevToolsAgentHost devtools, MatchCallback callback) {
        devtools.sendCommand("DOM.getDocument", "{ \"depth\": 1 }", (result, error) -> {
            if (result == null) {
                fail(callback);
                return;
            }
            JSONObject json = parseOrFail(result, callback);
            if (json == null) return;
            JSONObject root = json.optJSONObject("root");
            if (root == null) {
                fail(callback);
                return;
            }
            callback.onNodeMatch(root.optInt("nodeId", NO_MATCH));
        });
    }

    private void querySelectorAll(
            DevToolsAgentHost devtools, int rootId, String selector, MatchesCallback callback) {
        String params = null;
        try {
            JSONObject json = new JSONObject();
            json.put("nodeId", rootId);
            json.put("selector", selector);
            params = json.toString();
        } catch (Exception e) {
            fail(callback);
            return;
        }
        devtools.sendCommand("DOM.querySelectorAll", params, (result, error) -> {
            if (result == null) {
                fail(callback);
                return;
            }
            JSONObject json = parseOrFail(result, callback);
            if (json == null) return;
            JSONArray nodeIds = json.optJSONArray("nodeIds");
            if (nodeIds == null) {
                fail(callback);
                return;
            }
            int[] matches = new int[nodeIds.length()];
            for (int i = 0; i < nodeIds.length(); i++) {
                matches[i] = nodeIds.optInt(i, NO_MATCH);
                if (matches[i] == NO_MATCH) {
                    fail(callback);
                    return;
                }
            }
            callback.onNodeMatches(matches);
        });
    }

    private JSONObject parseOrNull(String json) {
        if (json == null) {
            return null;
        }
        try {
            return new JSONObject(json);
        } catch (Exception e) {
            return null;
        }
    }

    private JSONObject parseOrFail(String json, MatchCallback callback) {
        try {
            return new JSONObject(json);
        } catch (Exception e) {
            fail(callback);
            return null;
        }
    }

    private JSONObject parseOrFail(String json, MatchesCallback callback) {
        try {
            return new JSONObject(json);
        } catch (Exception e) {
            fail(callback);
            return null;
        }
    }

    private void fail(MatchCallback callback) {
        callback.onNodeMatch(NO_MATCH);
    }

    private void fail(MatchesCallback callback) {
        callback.onNodeMatches(NO_MATCHES);
    }

    private String makeSetValueFunction(String value) {
        JSONObject object = new JSONObject();
        try {
            object.put("value", value);
        } catch (Exception e) {
            return null;
        }
        return String.format(SET_VALUE_JS, object.toString());
    }
}
