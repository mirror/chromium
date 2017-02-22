// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import android.annotation.TargetApi;
import android.app.job.JobParameters;
import android.app.job.JobService;
import android.os.Build;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;

import java.util.HashMap;
import java.util.Map;

/** Delegates calls out to various tasks that need to run in the background. */
@TargetApi(Build.VERSION_CODES.LOLLIPOP_MR1)
class BackgroundTaskJobService extends JobService {
    private static final String TAG = "BkgrdTaskJS";

    private static class TaskFinishedCallbackJobService
            implements BackgroundTask.TaskFinishedCallback {
        private final BackgroundTaskJobService mJobService;
        private final JobParameters mParams;

        TaskFinishedCallbackJobService(BackgroundTaskJobService jobService, JobParameters params) {
            mJobService = jobService;
            mParams = params;
        }

        @Override
        public void taskFinished(final boolean needsReSchedule) {
            // Need to remove the current task from the currently running tasks. All other access
            // happens on the main thread, so do this removal also on the main thread.
            // To ensure that a new job is not immediately scheduled in between removing the task
            // from being a current task and before calling jobFinished, leading to us finishing
            // something with the same ID, call
            // {@link JobService#jobFinished(JobParameters, boolean} also on the main thread.
            ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    // FIXME what if this happens way later?
                    mJobService.mCurrentTasks.remove(mParams.getJobId());
                    mJobService.jobFinished(mParams, needsReSchedule);
                }
            });
        }
    }

    private final Map<Integer, BackgroundTask> mCurrentTasks = new HashMap<>();

    @Override
    public boolean onStartJob(JobParameters params) {
        ThreadUtils.assertOnUiThread();
        BackgroundTask backgroundTask =
                BackgroundTaskSchedulerJobService.getBackgroundTaskFromJobParameters(params);
        if (backgroundTask == null) {
            Log.w(TAG, "Failed to start job, because class does not exist.");
            return false;
        }

        mCurrentTasks.put(params.getJobId(), backgroundTask);

        TaskParameters taskParams =
                BackgroundTaskSchedulerJobService.getTaskParametersFromJobParameters(params);
        boolean taskNeedsBackgroundProcessing = backgroundTask.onStartTask(getApplicationContext(),
                taskParams, new TaskFinishedCallbackJobService(this, params));

        if (!taskNeedsBackgroundProcessing) mCurrentTasks.remove(params.getJobId());
        return taskNeedsBackgroundProcessing;
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        ThreadUtils.assertOnUiThread();
        if (!mCurrentTasks.containsKey(params.getJobId())) {
            Log.w(TAG, "Failed to stop job, because job with job id " + params.getJobId()
                            + " does not exist.");
            return false;
        }

        BackgroundTask backgroundTask = mCurrentTasks.get(params.getJobId());

        TaskParameters taskParams =
                BackgroundTaskSchedulerJobService.getTaskParametersFromJobParameters(params);
        boolean taskNeedsReSchedule =
                backgroundTask.onStopTask(getApplicationContext(), taskParams);
        mCurrentTasks.remove(params.getJobId());
        return taskNeedsReSchedule;
    }
}
