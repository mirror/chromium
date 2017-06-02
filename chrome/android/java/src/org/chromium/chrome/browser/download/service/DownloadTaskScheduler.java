// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.service;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.background_task_scheduler.BackgroundTaskScheduler;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;

import java.util.concurrent.TimeUnit;

/**
 *
 */
@JNINamespace("download::android")
public class DownloadTaskScheduler {
    @CalledByNative
    private static void scheduleDownloadTask(
            boolean requiresCharging, boolean requiresUnmeteredNetwork) {
        BackgroundTaskScheduler scheduler = BackgroundTaskSchedulerFactory.getScheduler();
        TaskInfo taskInfo = TaskInfo.createOneOffTask(TaskIds.DOWNLOAD_SERVICE_JOB_ID,
                                            DownloadBackgroundTask.class, 0)
                                    .setRequiredNetworkType(requiresUnmeteredNetwork
                                                    ? TaskInfo.NETWORK_TYPE_UNMETERED
                                                    : TaskInfo.NETWORK_TYPE_ANY)
                                    .setRequiresCharging(requiresCharging)
                                    .setUpdateCurrent(true)
                                    .setIsPersisted(true)
                                    .build();
        scheduler.schedule(ContextUtils.getApplicationContext(), taskInfo);
    }

    @CalledByNative
    private static void scheduleCleanupTask(long keepAliveTimeMs) {
        BackgroundTaskScheduler scheduler = BackgroundTaskSchedulerFactory.getScheduler();
        TaskInfo taskInfo = TaskInfo.createOneOffTask(TaskIds.DOWNLOAD_CLEANUP_JOB_ID,
                                            DownloadBackgroundTask.class, keepAliveTimeMs,
                                            keepAliveTimeMs + TimeUnit.HOURS.toMillis(1))
                                    .setRequiresCharging(true)
                                    .setIsPersisted(true)
                                    .build();
        scheduler.schedule(ContextUtils.getApplicationContext(), taskInfo);
    }

    /*@CalledByNative
    private static void cancelDownloadTask() {
        BackgroundTaskScheduler scheduler = BackgroundTaskSchedulerFactory.getScheduler();
        scheduler.cancel(ContextUtils.getApplicationContext(), TaskIds.DOWNLOAD_SERVICE_JOB_ID);
    }*/
}
