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
 * AwVariationsSeedFetcher is a JobService which periodically downloads the variations seed. There
 * are two versions of the job: metered (may run when there is any network access) and unmetered
 * (may run only with unmetered network access). Whenever either job runs, it re-schedules the other
 * job for one period in the future, to ensure both do not run in quick succession. The metered job
 * has a longer period than the unmetered job, to conserve data. The periods are chosen as a
 * trade-off between seed freshness (and prompt delivery of feature killswitches) and data and
 * battery usage. Various Android versions may enforce longer periods, depending on WebView usage
 * and battery-saving features. The jobs are first scheduled after boot when the first app requests
 * the seed from AwVariationsSeedService; if WebView is never used, the jobs will never run.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsSeedFetcher extends JobService {
    private static final String TAG = "AwVariationsSeedFet-";

    // TODO replace WEBVIEW_VARIATIONS_SEED_FETCH_JOB_ID
    /* package */ static final int NO_JOB_ID = 0;
    /* package */ static final int UNMETERED_JOB_ID = 1;
    /* package */ static final int METERED_JOB_ID = 2;
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
            Log.e("blarg", "schedule failed");
    }

    private static void scheduleUnmetered() {
        Log.e("blarg", "scheduling unmetered");
        JobInfo.Builder builder = new JobInfo.Builder(UNMETERED_JOB_ID, thisComponent());
        //builder.setPersisted(true);
        builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_UNMETERED);
        builder.setPeriodic(UNMETERED_PERIOD_MILLIS);
        schedule(builder.build());
        Log.e("blarg", "scheduled unmetered");
    }

    private static void scheduleMetered() {
        Log.e("blarg", "scheduling metered");
        JobInfo.Builder builder = new JobInfo.Builder(METERED_JOB_ID, thisComponent());
        builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
        builder.setPeriodic(METERED_PERIOD_MILLIS);
        schedule(builder.build());
        Log.e("blarg", "scheduled metered");
    }

    // Use JobScheduler.getPendingJob() if it's available. Otherwise, fall back to iterating over
    // all jobs to find the one we want.
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

    // Called every time the fetch job runs, and whenever AwVariationsSeedServer receives a request,
    // to schedule whataver jobs need to be scheduled:
    // - If either job is not scheduled (getPendingJob == null) because this is the first run, or
    //   because the job somehow failed to get scheduled, then schedule that job.
    // - Whenever the metered job runs (currentJobId == METERED_JOB_ID), re-schedule the unmetered
    //   job one unmetered period further in the future.
    // - Whenever the unmetered job runs (currentJobId == UNMETERED_JOB_ID), re-schedule the metered
    //   job one metered period further in the future.
    // This ensures that:
    // - As long as the unmetered job keeps running, the metered job will never run (because the
    //   unmetered job has a shorter period, and will always reschedule the metered job before the
    //   metered job can run.)
    // - If the periods happen to line up such that one job is scheduled to run right after the
    //   other, the first of those jobs will reschedule the other, so that the second will be
    //   delayed by one period before running.
    /* package */ static void scheduleNeededJobs(int currentJobId) {
        JobScheduler scheduler = getJobScheduler();
        if (currentJobId == METERED_JOB_ID || getPendingJob(scheduler, UNMETERED_JOB_ID) == null) {
            scheduleUnmetered();
        }
        if (currentJobId == UNMETERED_JOB_ID || getPendingJob(scheduler, METERED_JOB_ID) == null) {
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
            Log.e("blarg", "fetch isInterrupted=" + Thread.currentThread().isInterrupted());

            /*
            try {
                Thread.class.getDeclaredMethod("logInterrupts").invoke(Thread.currentThread());
            } catch(NoSuchMethodException e) {
                Log.e("blarg", "logInterrupts not found");
            } catch(Exception e) {
                throw new RuntimeException(e);
            }
            */

            try {
                SeedInfo newSeed = VariationsSeedFetcher.get().downloadContent(
                        VariationsSeedFetcher.VariationsPlatform.ANDROID_WEBVIEW,
                        "" /* restrictMode */, "63" /* blarg getMilestone() */, getChannel());
                Log.e("blarg", "fetch download done");

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
                /*
                try {
                    Log.e("blarg", "interrupts:");
                    for (Throwable t: (ArrayList<Throwable>) Thread.class.getDeclaredMethod("getInterrupts").invoke(Thread.currentThread())) {
                        Log.e("blarg", "interrupt", t);
                    }
                } catch(NoSuchMethodException e2) {
                    Log.e("blarg", "logInterrupts not found");
                } catch(Exception e2) {
                    throw new RuntimeException(e2);
                }
                */
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

    private long mStartTime;

    @Override
    public boolean onStartJob(JobParameters params) {
        mStartTime = android.os.SystemClock.elapsedRealtime();
        Log.e("blarg", "onStartJob id=" + params.getJobId());
        scheduleNeededJobs(params.getJobId());
        // Create mFetchTask - if this process has survived since the last run of this job,
        // mFetchTalk will still exist.
        if (mFetchTask == null) mFetchTask = new FetchTask(params);
        // Start the task if it's not aleady running - if this process has survived since the last
        // run of this job, it's unlikely but possible that the task is still running. (To prevent a
        // race condition between the condition and the execute(), mFetchTask must only be executeed
        // on the main thread.)
        if (mFetchTask.getStatus() != AsyncTask.Status.RUNNING) mFetchTask.execute();
        return true;
    }

    // JobParameters.class.getDeclaredMethod("getReasonName", int.class).invoke(reason);
    public static String getReasonName(int reason) {
        switch (reason) {
            case 0: return "canceled";
            case 1: return "constraints";
            case 2: return "preempt";
            case 3: return "timeout";
            case 4: return "device_idle";
            default: return "unknown: " + reason;
        }
    }

    private static String getStopReason(JobParameters params) {
        try {
            int reason = (Integer) JobParameters.class.getDeclaredMethod("getStopReason").invoke(params);
            return getReasonName(reason);
        } catch(Exception e) {
            throw new RuntimeException(e);
        }
    }

    private static String getDebugStopReason(JobParameters params) {
        try {
            return (String) JobParameters.class.getDeclaredMethod("getDebugStopReason").invoke(params);
        } catch(NoSuchMethodException e) {
            return "-";
        } catch(Exception e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        long elapsed = android.os.SystemClock.elapsedRealtime() - mStartTime;
        Log.e("blarg", "onStopJob id=" + params.getJobId() + " elapsed=" + elapsed +
                " reason=" + getStopReason(params) + " debug=" + getDebugStopReason(params));
        if (mFetchTask != null) {
            mFetchTask.cancel(true);
            mFetchTask = null;
        }
        return false;
    }
}
