// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feedback;

import android.content.Context;
import android.text.TextUtils;

import org.chromium.base.CollectionUtil;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.ChildProcessUtils;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Grabs feedback about the current system. */
@JNINamespace("chrome::android")
public class ProcessIdFeedbackSource
        extends AsyncFeedbackSourceAdapter < Map < String, List<Integer>>> {
    ProcessIdFeedbackSource() {}

    private static final String processTypeToFeedbackKey(String type) {
        return "Process IDs (" + type + ")";
    }

    // AsyncFeedbackSourceAdapter implementation.
    @Override
    protected Map<String, List<Integer>> doInBackground(Context context) {
        // TODO(dtrainor): Auto-generated method stub
        return null;
    }

    @Override
    public Map<String, String> getFeedback() {
        Map<String, List<Integer>> processes = ChildProcessUtils.getProcessTypePids();

        Map<String, String> feedback = new HashMap<>();
        CollectionUtil.forEach(processes, entry -> {
            String key = processTypeToFeedbackKey(entry.getKey());
            String pids = "";
            for (Integer pid : entry.getValue()) {
                if (!TextUtils.isEmpty(pids)) pids += ", ";
                pids += pid.toString();
            }
            feedback.put(key, pids);
        });

        feedback.put(processTypeToFeedbackKey("browser"), Long.toString(nativeGetCurrentPid()));

        return feedback;
    }

    private static native long nativeGetCurrentPid();
}