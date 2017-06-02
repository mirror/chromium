// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.service;

import android.content.Context;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.background_task_scheduler.NativeBackgroundTask;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.background_task_scheduler.TaskParameters;

/**
 *
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
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        int taskType = taskParameters.getExtras().getInt(DownloadTaskScheduler.EXTRA_TASK_TYPE);
        boolean incognito =
                taskParameters.getExtras().getBoolean(DownloadTaskScheduler.EXTRA_IS_INCOGNITO);
        Profile profile = incognito ? Profile.getLastUsedProfile().getOffTheRecordProfile()
                                    : Profile.getLastUsedProfile().getOriginalProfile();
        nativeStartBackgroundTask(profile, taskType);
        callback.taskFinished(false);
    }

    @Override
    protected boolean onStopTaskBeforeNativeLoaded(Context context, TaskParameters taskParameters) {
        return true;
    }

    @Override
    protected boolean onStopTaskWithNative(Context context, TaskParameters taskParameters) {
        return true;
    }

    @Override
    public void reschedule(Context context) {
        // TODO(shaktisahu): Called when OS asks us to reschedule the task again. Inform download
        // service to run the reschedule routine again.
    }

    private native void nativeStartBackgroundTask(Profile profile, int taskType);
}
