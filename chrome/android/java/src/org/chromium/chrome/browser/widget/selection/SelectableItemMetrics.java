// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.selection;

import android.os.Handler;
import android.support.v4.util.SimpleArrayMap;

import org.chromium.base.metrics.RecordHistogram;

/**
 * Keep track of actions on a single selectable item.
 */
public class SelectableItemMetrics {
    /** The maximum duration that the count of single removal should be record. */
    private static final int SINGLE_REMOVAL_TIMEOUT_MS = 5000;

    private static SelectableItemMetrics sInstance;

    private final Handler mHandler;
    private final SimpleArrayMap<String, Integer> mSingleActionCounts;

    public static SelectableItemMetrics getInstance() {
        if (sInstance == null) {
            sInstance = new SelectableItemMetrics();
        }
        return sInstance;
    }

    private SelectableItemMetrics() {
        mHandler = new Handler();
        mSingleActionCounts = new SimpleArrayMap<>();
    }

    /**
     * Update the count of the specified action on a single selectable item. Record the count to
     * histogram for a certain time frame.
     * @param action The action of which the count should be updated.
     */
    public void recordSingleAction(String action) {
        if (mSingleActionCounts.get(action) == null) {
            mSingleActionCounts.put(action, 0);
        }

        if (mSingleActionCounts.get(action) == 0) {
            mSingleActionCounts.put(action, 1);
            mHandler.postDelayed(() -> {
                RecordHistogram.recordCountHistogram(action, mSingleActionCounts.get(action));
                mSingleActionCounts.put(action, 0);
            }, SINGLE_REMOVAL_TIMEOUT_MS);
        } else {
            final int count = mSingleActionCounts.get(action);
            mSingleActionCounts.put(action, count + 1);
        }
    }
}
