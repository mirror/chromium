// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feedback;

import android.text.TextUtils;

import org.chromium.base.CollectionUtil;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.ChildProcessUtils;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Grabs feedback about the current system. */
@JNINamespace("chrome::android")
public class ProcessIdFeedbackSource implements AsyncFeedbackSource {
    private boolean mComplete;
    private Map<String, List<Integer>> mProcessMap;

    ProcessIdFeedbackSource() {}

    private static final String processTypeToFeedbackKey(String type) {
        return "Process IDs (" + type + ")";
    }

    // AsyncFeedbackSource implementation.
    @Override
    public void start(final Runnable callback) {
        ChildProcessUtils.getProcessIdsByType(results -> {
            mProcessMap = results;
            mComplete = true;
            callback.run();
        });
    }

    @Override
    public boolean isReady() {
        return mComplete;
    }

    @Override
    public Map<String, String> getFeedback() {
        Map<String, String> feedback = new HashMap<>();
        if (mProcessMap != null) {
            CollectionUtil.forEach(mProcessMap, entry -> {
                String key = processTypeToFeedbackKey(entry.getKey());
                String pids = "";
                for (Integer pid : entry.getValue()) {
                    if (!TextUtils.isEmpty(pids)) pids += ", ";
                    pids += pid.toString();
                }
                feedback.put(key, pids);
            });
        }

        feedback.put(processTypeToFeedbackKey("browser"), Long.toString(nativeGetCurrentPid()));

        return feedback;
    }

    private static native long nativeGetCurrentPid();
}