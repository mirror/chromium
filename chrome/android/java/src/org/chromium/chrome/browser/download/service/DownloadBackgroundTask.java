// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.service;

import android.content.Context;

import org.chromium.base.Callback;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.background_task_scheduler.NativeBackgroundTask;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.background_task_scheduler.TaskParameters;

/**
 * Entry point for the download service to perform desired action when the task is fired by the
 * scheduler. The scheduled task is executed for the regular profile and also for incognito profile
 * if an incognito profile exists.
 */
@JNINamespace("download::android")
public class DownloadBackgroundTask extends NativeBackgroundTask {
    @Override
    protected int onStartTaskBeforeNativeLoaded(
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        return NativeBackgroundTask.LOAD_NATIVE;
    }

    @Override
    protected void onStartTaskWithNative(
            Context context, TaskParameters taskParameters, final TaskFinishedCallback callback) {
        int taskType = taskParameters.getExtras().getInt(DownloadTaskScheduler.EXTRA_TASK_TYPE);
        Profile profile = Profile.getLastUsedProfile().getOriginalProfile();
        nativeStartBackgroundTask(profile, taskType, new Callback<Boolean>() {
            @Override
            public void onResult(Boolean needsReschedule) {
                callback.taskFinished(needsReschedule);
            }
        });
    }

    @Override
    protected boolean onStopTaskBeforeNativeLoaded(Context context, TaskParameters taskParameters) {
        return true;
    }

    @Override
    protected boolean onStopTaskWithNative(Context context, TaskParameters taskParameters) {
        int taskType = taskParameters.getExtras().getInt(DownloadTaskScheduler.EXTRA_TASK_TYPE);
        Profile profile = Profile.getLastUsedProfile().getOriginalProfile();
        return nativeStopBackgroundTask(profile, taskType);
    }

    @Override
    public void reschedule(Context context) {
        // TODO(shaktisahu): Called when system asks us to schedule the task again. (crbug/730786)
    }

    private native void nativeStartBackgroundTask(
            Profile profile, int taskType, Callback<Boolean> callback);
    private native boolean nativeStopBackgroundTask(Profile profile, int taskType);
}
