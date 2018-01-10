// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import org.chromium.android_webview.AwBrowserProcess;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;

import org.chromium.android_webview.AwVariationsUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;

/**
 * AwVariationsSeedFetcher is a Job Service to fetch test seed data which is used by Variations
 * to enable AB testing experiments in the native code. The fetched data is stored in WebView's data
 * directory. The tryLock mechanism is just to make sure that
 * no seed fetch job will be scheduled when there is one still running. This is a prototype of the
 * Variations Seed Fetch Service which is one part of the work of adding Variations to Android
 * WebView.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsSeedFetcher extends JobService {
    private static final String TAG = "AwVariationsSeedFet-";

    // TODO replace WEBVIEW_VARIATIONS_SEED_FETCH_JOB_ID
    public static final int UNMETERED_JOB_ID = 0;
    public static final int METERED_JOB_ID = 1;
    //private static final long HOUR_MILLIS = 60 * 60 * 1000;
    //private static final long DAY_MILLIS = 24 * HOUR_MILLIS;
    private static final long UNMETERED_PERIOD_MILLIS = 1000;
    private static final long METERED_PERIOD_MILLIS = 3000;

    private AwVariationsSeedHolder mSeedHolder;
    private FetchTask mFetchTask;

    private static ComponentName thisComponent() {
        return new ComponentName(ContextUtils.getApplicationContext(),
                AwVariationsSeedFetcher.class);
    }

    private static JobScheduler getJobScheduler() {
        return (JobScheduler) ContextUtils.getApplicationContext()
                .getSystemService(Context.JOB_SCHEDULER_SERVICE);
    }

    private static void schedule(JobInfo info) {
        if (getJobScheduler().schedule(info) != JobScheduler.RESULT_SUCCESS)
            Log.i("blarg", "schedule failed");
    }

    private static void scheduleUnmetered() {
        JobInfo.Builder builder = new JobInfo.Builder(UNMETERED_JOB_ID, thisComponent());
        //builder.setPersisted(true);
        builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_UNMETERED);
        builder.setPeriodic(UNMETERED_PERIOD_MILLIS);
        schedule(builder.build());
    }

    private static void scheduleMetered() {
        JobInfo.Builder builder = new JobInfo.Builder(METERED_JOB_ID, thisComponent());
        builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
        builder.setMinimumLatency(METERED_PERIOD_MILLIS);
        schedule(builder.build());
    }

    @SuppressLint("NewApi")
    private static JobInfo getPendingJob(JobScheduler scheduler, int jobId) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
            for (JobInfo info: scheduler.getAllPendingJobs()) {
                if (info.getId() == jobId) return info;
            }
            return null;
        }

        return scheduler.getPendingJob(jobId);
    }

    /* package */ static void scheduleIfNeeded() {
        JobScheduler scheduler = getJobScheduler();
        if (getPendingJob(scheduler, UNMETERED_JOB_ID) == null) {
            scheduleUnmetered();
        }
        if (getPendingJob(scheduler, METERED_JOB_ID) == null) {
            scheduleMetered();
        }
    }

    private String getMilestone() {
        try {
            String versionName = getPackageManager().getPackageInfo(getPackageName(), 0)
                    .versionName;
            int firstDot = versionName.indexOf('.');
            if (firstDot < 1) {
                Log.e("blarg", "getMilestone malformed version name: " + versionName);
                return "";
            }
            return versionName.substring(0, firstDot);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e("blarg", "getMilestone could not find own package name");
            return "";
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
        return "";
    }

    private class FetchTask extends AsyncTask<Void, Void, Void> {
        private JobParameters mParams;

        FetchTask(JobParameters params) {
            mParams = params;
        }

        @Override
        protected Void doInBackground(Void... unused) {
            Log.e("blarg", "fetch");

            try {
                SeedInfo newSeed = VariationsSeedFetcher.get().downloadContent(
                        VariationsSeedFetcher.VariationsPlatform.ANDROID_WEBVIEW,
                        "" /* restrictMode */, getMilestone(), getChannel());

                if (isCancelled()) return null;

                File newSeedFile = AwVariationsUtils.getNewSeedFile();
                // createNewFile() silently returns false if the file already exists, which could be
                // the case if a previous FetchTask died before deleting it.
                newSeedFile.createNewFile();
                AwVariationsUtils.writeSeedToStream(new FileOutputStream(newSeedFile), newSeed);
                mSeedHolder.updateSeed(newSeed, newSeedFile);
                newSeedFile.delete();
            } catch (IOException e) {
                // downloadContent() logs and re-throws IOExceptions, so there's no need to log here.
            }

            jobFinished(mParams, false /* needsReschedule */);
            return null;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        ContextUtils.initApplicationContext(this.getApplicationContext());
        PathUtils.setPrivateDataDirectorySuffix(AwBrowserProcess.PRIVATE_DATA_DIRECTORY_SUFFIX);
        mSeedHolder = AwVariationsSeedHolder.getInstance();
    }

    @Override
    public boolean onStartJob(JobParameters params) {
        Log.e("blarg", "onStartJob");
        scheduleMetered();
        if (mFetchTask == null) mFetchTask = new FetchTask(params);
        mFetchTask.execute();
        return true;
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        if (mFetchTask != null) {
            mFetchTask.cancel(true);
            mFetchTask = null;
        }
        return false;
    }
}
