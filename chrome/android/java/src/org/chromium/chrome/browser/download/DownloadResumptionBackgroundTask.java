// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.Context;
import android.content.Intent;

import org.chromium.components.background_task_scheduler.BackgroundTask;
import org.chromium.components.background_task_scheduler.TaskParameters;

/**
 * An implementation of BackgroundTask that is responsible for resuming any in-progress downloads.
 * This class currently just starts the {@link DownloadNotificationService}, which handles the
 * actual resumption.
 */
public class DownloadResumptionBackgroundTask implements BackgroundTask {
    // BackgroundTask implementation.
    @Override
    public boolean onStartTask(
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        // Start the DownloadNotificationService and allow that to manage the download life cycle.
        // Shut down the task right away after starting the service
        DownloadNotificationService.startDownloadNotificationService(
                context, new Intent(DownloadNotificationService.ACTION_DOWNLOAD_RESUME_ALL));
        return false;
    }

    @Override
    public boolean onStopTask(Context context, TaskParameters taskParameters) {
        // The task is not necessary once started, so no need to auto-resume here.  The started
        // service will handle rescheduling the job if necessary.
        return false;
    }

    @Override
    public void reschedule(Context context) {
        DownloadResumptionScheduler.getDownloadResumptionScheduler(context).scheduleIfNecessary();
    }
}