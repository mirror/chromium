// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import org.json.JSONArray;
import org.json.JSONObject;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.assistant.assist_actions.AssistAction;

import java.util.ArrayList;

/**
 * Client for the remote Web Assistant service.
 *
 * Sends the current tab state (URL, etc) as context and receives a list of
 * actions that the Assistant can perform on that tab.
 */
class WebAssistantService {
    public static interface ActionsCallback {
        public void onActionsForUrl(String url, ArrayList<AssistAction> actions);
    }

    private long mNative;

    public WebAssistantService(String serviceUrl) {
        mNative = nativeInit(serviceUrl);
    }

    public void close() {
        if (mNative != 0) {
            nativeDestroy(mNative);
            mNative = 0;
        }
    }

    public void getActionsForUrl(String url, ActionsCallback callback) {
        if (mNative != 0) {
            nativeGetActionsForUrl(mNative, url, callback);
        }
    }

    @CalledByNative
    private void onActionsForUrlResponse(
            String url, String jsonResponse, ActionsCallback callback) {
        JSONObject response = null;
        try {
            response = new JSONObject(jsonResponse);
        } catch (Exception e) {
            // Ignore.
            return;
        }

        ArrayList<AssistAction> actions = new ArrayList<AssistAction>();

        JSONArray array = response.optJSONArray("action");
        if (array != null) {
            for (int i = 0; i < array.length(); i++) {
                JSONObject json = array.optJSONObject(i);
                if (json != null) {
                    AssistAction action = AssistAction.parse(json);
                    if (action != null) {
                        actions.add(action);
                    }
                }
            }
        }

        callback.onActionsForUrl(url, actions);
    }

    private native long nativeInit(String serviceUrl);
    private native void nativeDestroy(long nativeWebAssistantServiceAndroid);
    private native void nativeGetActionsForUrl(
            long nativeWebAssistantServiceAndroid, String url, ActionsCallback callback);
}
