// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.app.Activity;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.Intent;
import android.os.Bundle;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.chrome.browser.customtabs.CustomTabsConnection;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.metrics.UmaUtils;
import org.chromium.chrome.browser.profiles.ProfileManagerUtils;
import org.chromium.chrome.browser.util.IntentUtils;

import java.lang.ref.WeakReference;
import java.util.List;

/**
 * Base class for FirstRunActivity and LightweightFirstRunActivity. Contains logic which is common
 * to both implementations.
 */
public abstract class FirstRunActivityBase extends AsyncInitializationActivity {
    private static final String TAG = "FirstRunActivity";

    private boolean mNativeInitialized;

    @Override
    protected boolean requiresFirstRunToBeCompleted(Intent intent) {
        // The user is already in First Run.
        return false;
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    // Activity:

    @Override
    public void onPause() {
        super.onPause();
        flushPersistentData();
    }

    @Override
    public void onStart() {
        super.onStart();
        // Since the FRE may be shown before any tab is shown, mark that this is the point at
        // which Chrome went to foreground. This is needed as otherwise an assert will be hit
        // in UmaUtils.getForegroundStartTime() when recording the time taken to load the first
        // page (which happens after native has been initialized possibly while FRE is still
        // active).
        UmaUtils.recordForegroundStartTime();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        mNativeInitialized = true;
    }

    protected void flushPersistentData() {
        if (mNativeInitialized) {
            ProfileManagerUtils.flushPersistentDataForAllProfiles();
        }
    }

    /**
     * Finish all the instances of the given Activity.
     * @param targetActivity The class name of the target Activity.
     */
    protected static void finishAllTheActivities(String targetActivity) {
        List<WeakReference<Activity>> activities = ApplicationStatus.getRunningActivities();
        for (WeakReference<Activity> weakActivity : activities) {
            Activity activity = weakActivity.get();
            if (activity != null && activity.getLocalClassName().equals(targetActivity)) {
                activity.finish();
            }
        }
    }

    /**
     * Sends the PendingIntent included with the CHROME_LAUNCH_INTENT extra if it exists.
     * @param complete Whether first run completed successfully.
     * @return Whether a pending intent was sent.
     */
    protected final boolean sendPendingIntentIfNecessary(final boolean complete) {
        PendingIntent pendingIntent = IntentUtils.safeGetParcelableExtra(
                getIntent(), FirstRunActivity.EXTRA_CHROME_LAUNCH_INTENT);
        if (pendingIntent == null) return false;

        Intent extraDataIntent = new Intent();
        extraDataIntent.putExtra(FirstRunActivity.EXTRA_FIRST_RUN_ACTIVITY_RESULT, true);
        extraDataIntent.putExtra(FirstRunActivity.EXTRA_FIRST_RUN_COMPLETE, complete);

        try {
            // After the PendingIntent has been sent, send a first run callback to custom tabs if
            // necessary.
            PendingIntent.OnFinished onFinished = new PendingIntent.OnFinished() {
                @Override
                public void onSendFinished(PendingIntent pendingIntent, Intent intent,
                        int resultCode, String resultData, Bundle resultExtras) {
                    if (ChromeLauncherActivity.isCustomTabIntent(intent)) {
                        CustomTabsConnection.getInstance().sendFirstRunCallbackIfNecessary(
                                intent, complete);
                    }
                }
            };

            // Use the PendingIntent to send the intent that originally launched Chrome. The intent
            // will go back to the ChromeLauncherActivity, which will route it accordingly.
            pendingIntent.send(this, complete ? Activity.RESULT_OK : Activity.RESULT_CANCELED,
                    extraDataIntent, onFinished, null);
            return true;
        } catch (CanceledException e) {
            Log.e(TAG, "Unable to send PendingIntent.", e);
        }
        return false;
    }
}
