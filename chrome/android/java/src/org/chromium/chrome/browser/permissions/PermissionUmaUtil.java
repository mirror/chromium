// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.permissions;

import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.metrics.RecordHistogram;

/**
 */
public class PermissionUmaUtil {
    private static final int BATTERY_BUCKETS = 11;
    /**
     * Log an action with the current battery level, bucketed as 0%-9%, 10%-19%, ..., 100%.
     */
    @CalledByNative
    private static void recordWithBatteryBucket(String histogram) {
        IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent batteryStatus = ContextUtils.getApplicationContext().registerReceiver(null, ifilter);
        int current = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        int max = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
        double percentage = 100.0 * current / max;
        int bucket = (int) (percentage / (BATTERY_BUCKETS - 1));
        assert 0 <= bucket && bucket < BATTERY_BUCKETS;

        RecordHistogram.recordEnumeratedHistogram(histogram, bucket, BATTERY_BUCKETS);
    }
}
