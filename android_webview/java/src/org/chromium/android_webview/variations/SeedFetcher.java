// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Build;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

/**
 * SeedFetcher is a JobService which periodically downloads the variations seed. We use JobService
 * instead of BackgroundTaskScheduler; BackgroundTaskScheduler exists to support pre-L, and
 * internally uses JobService on L+. WebView is L+ only, so can use JobService directly. The fetch
 * period is chosen as a trade-off between seed freshness (and prompt delivery of feature
 * killswitches) and data and battery usage. Various Android versions may enforce longer periods,
 * depending on WebView usage and battery-saving features. The jobs are first scheduled after boot
 * when the first app requests the seed from AwVariationsSeedService; if WebView is never used, the
 * jobs will never run. SeedFetcher is not meant to be used outside the service.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // for JobService
public class SeedFetcher extends JobService {
    private static final String TAG = "SeedFetcher";

    private static final int JOB_ID = TaskIds.WEBVIEW_VARIATIONS_SEED_FETCH_JOB_ID;
    private static final long JOB_PERIOD_MILLIS = TimeUnit.DAYS.toMillis(1);

    private SeedHolder mSeedHolder;
    private FetchTask mFetchTask;

    // Use JobScheduler.getPendingJob() if it's available. Otherwise, fall back to iterating over
    // all jobs to find the one we want.
    @SuppressLint("NewApi")
    private static JobInfo getPendingJob(JobScheduler scheduler, int jobId) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
            for (JobInfo info : scheduler.getAllPendingJobs()) {
                if (info.getId() == jobId) return info;
            }
            return null;
        }

        return scheduler.getPendingJob(jobId);
    }

    /* package */ static void scheduleIfNeeded() {
        JobScheduler scheduler = (JobScheduler) ContextUtils.getApplicationContext()
                .getSystemService(Context.JOB_SCHEDULER_SERVICE);

        // Check if it's already scheduled.
        if (getPendingJob(scheduler, JOB_ID) != null) {
            return;
        }

        ComponentName thisComponent =
                new ComponentName(ContextUtils.getApplicationContext(), SeedFetcher.class);
        JobInfo.Builder builder = new JobInfo.Builder(JOB_ID, thisComponent);
        builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
        builder.setPeriodic(JOB_PERIOD_MILLIS);
        if (scheduler.schedule(builder.build()) != JobScheduler.RESULT_SUCCESS) {
            Log.e(TAG, "Failed to schedule job");
        }
    }

    private String getMilestone() {
        try {
            // versionName should be e.g. "64.0.3282.137".
            String versionName =
                    getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
            // Slice the milestone e.g. "64" out of versionName.
            int firstDot = versionName.indexOf('.');
            if (firstDot < 1) {
                Log.w(TAG, "Cannot extract milestone from version name: \"" + versionName + "\"");
                return null;
            }
            return versionName.substring(0, firstDot);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Could not find own package");
            return null;
        }
    }

    // This is copied from version_info::ChannelFromPackageName(), because ChannelFromPackageName()
    // is native, and the Finch service doesn't load native. TODO(paulmiller): We may want to move
    // the version_info one to Java in order to de-duplicate them. We may also want to de-duplicate
    // the return values with AsyncInitTaskRunner$FetchSeedTask.getChannelString().
    private String getChannel() {
        String packageName = getPackageName();
        if ("com.android.chrome".equals(packageName)) return "stable";
        if ("com.chrome.beta".equals(packageName)) return "beta";
        if ("com.chrome.dev".equals(packageName)) return "dev";
        if ("com.chrome.canary".equals(packageName)) return "canary";
        return null; // Channel is unknown for stand-alone WebView.
    }

    private class FetchTask extends AsyncTask<Void, Void, Void> {
        private JobParameters mParams;

        FetchTask(JobParameters params) {
            mParams = params;
        }

        @Override
        protected Void doInBackground(Void... unused) {
            try {
                SeedInfo newSeed = VariationsSeedFetcher.get().downloadContent(
                        VariationsSeedFetcher.VariationsPlatform.ANDROID_WEBVIEW,
                        /*restrictMode=*/null, getMilestone(), getChannel());

                if (isCancelled()) return null;

                mSeedHolder.updateSeed(newSeed,
                        /*onFinished=*/() -> jobFinished(mParams, /*needsReschedule=*/false));
            } catch (IOException e) {
                // downloadContent() logs and re-throws IOExceptions, so there's no need to log
                // here. IOException includes InterruptedIOException, which may happen inside
                // downloadContent() if the job is cancelled in onStopJob().
            }
            return null;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        ContextUtils.initApplicationContext(this.getApplicationContext());
        PathUtils.setPrivateDataDirectorySuffix(AwBrowserProcess.WEBVIEW_DIR_BASENAME);
        mSeedHolder = SeedHolder.getInstance();
    }

    @Override
    public boolean onStartJob(JobParameters params) {
        // If this process has survived since the last run of this job, mFetchTalk will still exist.
        // Otherwise, create it.
        if (mFetchTask == null) mFetchTask = new FetchTask(params);
        // Start the task if it's not aleady running - if this process has survived since the last
        // run of this job, it's very unlikely but possible that the task is still running.
        if (mFetchTask.getStatus() != AsyncTask.Status.RUNNING) mFetchTask.execute();
        return true;
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        if (mFetchTask != null) {
            mFetchTask.cancel(true);
        }
        return false;
    }
}
