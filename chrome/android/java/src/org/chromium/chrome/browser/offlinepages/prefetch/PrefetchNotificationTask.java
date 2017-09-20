// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages.prefetch;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.background_task_scheduler.BackgroundTask;
import org.chromium.components.background_task_scheduler.BackgroundTask.TaskFinishedCallback;
import org.chromium.components.background_task_scheduler.BackgroundTaskScheduler;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;

/**
 * Handles polling for offline status and displaying notification when offline for some time.
 */
public class PrefetchNetworkStatusTask implements BackgroundTask {
    /**
     * Key used to save the date on which the last notification was shown.
     */
    private static final String PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_TIMESTAMP =
            "offline_pages_prefetch_notification_timestamp";
    private static final String PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_HAS_NEW_PAGES =
            "offline_pages_prefetch_notification_has_new_pages";
    private static final String PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_OFFLINE_COUNTER =
            "offline_pages_prefetch_notification_offline_counter";
    private static final String PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_IGNORED_COUNTER =
            "offline_pages_prefetch_notification_ignored_counter";

    private static final int MAX_IGNORED_NOTIFICATION = 3;
    private static final int MAX_OFFLINE_SAMPLE = 4;

    private static final long HOUR_INTERVAL_START = 57 * 60 * 1000; // 57 minutes
    private static final long HOUR_INTERVAL_END = 62 * 60 * 1000; // 62 minutes

    private static final long INCREMENTAL_INTERVAL_START = 12 * 60 * 1000; // 12 minutes
    private static final long INCREMENTAL_INTERVAL_END = 17 * 60 * 1000; // 17 minutes

    @Override
    public boolean onStartTask(
            Context context, TaskParameters params, TaskFinishedCallback callback) {
        ConnectivityManager cm =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo network = cm.getActiveNetworkInfo();
        SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor prefEditor = preferences.edit();

        BackgroundTaskScheduler scheduler = BackgroundTaskSchedulerFactory.getScheduler();

        // TODO: also check UserPreferences here.
        // Do not schedule if we have no new content nor
        if (preferences.getInt(PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_IGNORED_COUNTER)
                        > MAX_IGNORED_NOTIFICATION
                || !preferences.getBoolean(
                           PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_HAS_NEW_PAGES)) {
            return false;
        }

        if (network == null || network.getState != NetworkInfo.State.CONNECTED) {
            // Count how many times this has been.
            int currentOfflineCounter =
                    preferences.getInt(PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_OFFLINE_COUNTER);
            if (currentOfflineCounter > MAX_OFFLINE_SAMPLE) {
                // TODO: Start task to check native side.

                // Reset counters.
                prefEditor.putInt(PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_OFFLINE_COUNTER, 0);
                prefEditor.putBoolean(
                        PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_HAS_NEW_PAGES, false);
            } else {
                // If we haven't reached enough sample sizes, increment, and reschedule.
                prefEditor.putInt(
                        PREF_OFFLINE_PAGES_NOTIFICATION_OFFLINE_COUNTER, currentOfflineCounter + 1);
                scheduler.schedule(context,
                        TaskInfo.createOneOffTask(TaskIds.OFFLINE_PAGES_PREFETCH_NETWORK_JOB_ID,
                                PrefetchNetworkStatusTask.class, INCREMENTAL_INTERVAL_START,
                                INCREMENTAL_INTERVAL_END));
            }

        } else {
            // Reset offline sampling counter and reshedule for an hour.
            prefEditor.putInt(PREF_OFFLINE_PAGES_PREFETCH_NOTIFICATION_OFFLINE_COUNTER, 0);
            scheduler.schedule(context,
                    TaskInfo.createOneOffTask(TaskIds.OFFLINE_PAGES_PREFETCH_NETWORK_JOB_ID,
                            PrefetchNetworkStatusTask.class, HOUR_INTERVAL_START,
                            HOUR_INTERVAL_END));
        }
        return false;
    }

    @Override
    public boolean onStopTask(Context context, TaskParameters params) {}

    @Override
    public void reschedule(Context context) {}
}
