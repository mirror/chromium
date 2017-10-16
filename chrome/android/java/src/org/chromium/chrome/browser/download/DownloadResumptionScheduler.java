// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.annotation.SuppressLint;
import android.content.Context;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;
import org.chromium.components.background_task_scheduler.TaskInfo.NetworkType;

import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Class for scheduing download resumption tasks.
 */
public class DownloadResumptionScheduler {
    private final Context mContext;
    @SuppressLint("StaticFieldLeak")
    private static DownloadResumptionScheduler sDownloadResumptionScheduler;

    @SuppressFBWarnings("LI_LAZY_INIT")
    public static DownloadResumptionScheduler getDownloadResumptionScheduler(Context context) {
        assert context == context.getApplicationContext();
        if (sDownloadResumptionScheduler == null) {
            sDownloadResumptionScheduler = new DownloadResumptionScheduler(context);
        }
        return sDownloadResumptionScheduler;
    }

    protected DownloadResumptionScheduler(Context context) {
        mContext = context;
    }

    /**
     * For tests only: sets the DownloadResumptionScheduler.
     * @param scheduler An instance of DownloadResumptionScheduler.
     */
    @VisibleForTesting
    public static void setDownloadResumptionScheduler(DownloadResumptionScheduler scheduler) {
        sDownloadResumptionScheduler = scheduler;
    }

    /**
     * Checks the persistence layer and schedules a task to restart the app and resume any downloads
     * if there are resumable downloads available.
     */
    public void scheduleIfNecessary() {
        List<DownloadSharedPreferenceEntry> entries =
                DownloadSharedPreferenceHelper.getInstance().getEntries();

        boolean scheduleAutoResumption = false;
        boolean allowMeteredConnection = false;
        for (int i = 0; i < entries.size(); ++i) {
            DownloadSharedPreferenceEntry entry = entries.get(i);
            if (entry.isAutoResumable) {
                scheduleAutoResumption = true;
                if (entry.canDownloadWhileMetered) {
                    allowMeteredConnection = true;
                    break;
                }
            }
        }

        if (scheduleAutoResumption) {
            @NetworkType
            int networkType = allowMeteredConnection ? TaskInfo.NETWORK_TYPE_ANY
                                                     : TaskInfo.NETWORK_TYPE_UNMETERED;

            TaskInfo task = TaskInfo.createOneOffTask(TaskIds.DOWNLOAD_RESUMPTION_JOB_ID,
                                            DownloadResumptionBackgroundTask.class,
                                            TimeUnit.DAYS.toMillis(1))
                                    .setUpdateCurrent(true)
                                    .setRequiredNetworkType(networkType)
                                    .setRequiresCharging(false)
                                    .setIsPersisted(true)
                                    .build();

            BackgroundTaskSchedulerFactory.getScheduler().schedule(mContext, task);
        } else {
            cancel();
        }
    }

    /**
     * Cancels any outstanding task that could restart the app and resume downloads.
     */
    public void cancel() {
        BackgroundTaskSchedulerFactory.getScheduler().cancel(
                mContext, TaskIds.DOWNLOAD_RESUMPTION_JOB_ID);
    }
}