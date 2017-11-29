// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feedback;

import android.util.Pair;

import org.chromium.base.CollectionUtil;
import org.chromium.chrome.browser.AppHooks;

import java.util.Map;

/** Grabs feedback about the current system. */
public class DeviceCertifiedFeedbackSource implements AsyncFeedbackSource {
    /** {@code null} if the source was unable to be determined. */
    private Boolean mIsCertified;
    private boolean mIsReady;

    DeviceCertifiedFeedbackSource() {}

    // AsyncFeedbackSource implementation.
    @Override
    public boolean isReady() {
        return mIsReady;
    }

    @Override
    public void start(Runnable callback) {
        AppHooks.get().checkIfDeviceIsCertified(result -> {
            mIsReady = true;
            mIsCertified = result;
            callback.run();
        });
    }

    @Override
    public Map<String, String> getFeedback() {
        if (mIsCertified == null) return null;

        return CollectionUtil.newHashMap(Pair.create("Device Certified", mIsCertified.toString()));
    }
}