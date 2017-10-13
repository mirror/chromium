// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.StrictModeContext;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.background_task_scheduler.NativeBackgroundTask;
import org.chromium.chrome.browser.banners.InstallerDelegate;
import org.chromium.components.background_task_scheduler.BackgroundTask.TaskFinishedCallback;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskParameters;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

/**
 * Handles servicing of background WebAPK update requests coming via background_task_scheduler
 * component. Will update multiple WebAPKs if there are multiple WebAPKs pending update.
 */
public class WebApkUpdateTask extends NativeBackgroundTask {
    /** Whether the task was stopped. */
    private boolean mStopped = false;

    /** The WebappDataStorage for the WebAPK to update. */
    private WebappDataStorage mStorageToUpdate = null;

    @Override
    @StartBeforeNativeResult
    protected int onStartTaskBeforeNativeLoaded(
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        assert taskParameters.getTaskId() == TaskIds.WEBAPK_UPDATE_JOB_ID;

        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            WebappRegistry.warmUpSharedPrefs();
        }

        ArrayList<String> ids = findInstalledWebApksWithPendingUpdate();
        for (String id : ids) {
            WebappDataStorage storage = WebappRegistry.getInstance().getWebappDataStorage(id);
            if (!isWebApkActivityRunning(storage.getWebApkPackageName())) {
                mStorageToUpdate = storage;
                return LOAD_NATIVE;
            }
        }
        return ids.isEmpty() ? DONE : RESCHEDULE;
    }

    @Override
    protected void onStartTaskWithNative(
            Context context, TaskParameters taskParameters, final TaskFinishedCallback callback) {
        assert taskParameters.getTaskId() == TaskIds.WEBAPK_UPDATE_JOB_ID;

        WebApkUpdateManager updateManager = new WebApkUpdateManager(mStorageToUpdate);
        Runnable nextUpdateRunner = new Runnable() {
            @Override
            public void run() {
                boolean moreToUpdate = !findInstalledWebApksWithPendingUpdate().isEmpty();
                callback.taskFinished(!mStopped && moreToUpdate);
            }
        };
        updateManager.updateNow(nextUpdateRunner);
    }

    @Override
    protected boolean onStopTaskBeforeNativeLoaded(Context context, TaskParameters taskParameters) {
        assert taskParameters.getTaskId() == TaskIds.WEBAPK_UPDATE_JOB_ID;

        // Native didn't complete loading, but it was supposed to. Presume that we need to
        // reschedule.
        return true;
    }

    @Override
    protected boolean onStopTaskWithNative(Context context, TaskParameters taskParameters) {
        assert taskParameters.getTaskId() == TaskIds.WEBAPK_UPDATE_JOB_ID;

        mStopped = true;
        // Updating a single WebAPK is a fire and forget task. However, there might be several
        // WebAPKs that we need to update.
        return true;
    }

    @Override
    public void reschedule(Context context) {}

    /** Returns whether a WebApkActivity with {@link webApkPackageName} is running. */
    private static boolean isWebApkActivityRunning(String webApkPackageName) {
        for (WeakReference<Activity> activity : ApplicationStatus.getRunningActivities()) {
            if (!(activity.get() instanceof WebApkActivity)) {
                continue;
            }
            WebApkActivity webApkActivity = (WebApkActivity) activity.get();
            if (webApkPackageName.equals(webApkActivity.getWebApkPackageName())) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns list of WebAPKs which have pending updates. Filters out WebAPKs which have been
     * uninstalled.
     */
    private ArrayList<String> findInstalledWebApksWithPendingUpdate() {
        List<String> ids = WebappRegistry.getInstance().findWebApksWithPendingUpdate();
        // Filter out WebAPKs which have been uninstalled. A user may have uninstalled a WebAPK
        // between the time that the update was requested and the time that BackgroundTaskScheduler
        // runs the update task.
        ArrayList<String> installedIds = new ArrayList<String>();
        PackageManager packageManager = ContextUtils.getApplicationContext().getPackageManager();
        for (String id : ids) {
            WebappDataStorage storage = WebappRegistry.getInstance().getWebappDataStorage(id);
            if (InstallerDelegate.isInstalled(packageManager, storage.getWebApkPackageName())) {
                installedIds.add(id);
            }
        }
        return installedIds;
    }
}
