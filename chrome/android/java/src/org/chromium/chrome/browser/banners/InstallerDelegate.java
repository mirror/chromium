// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.banners;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.base.WindowAndroid;

/**
 * Monitors the app installation process and informs an observer of updates.
 */
public class InstallerDelegate {
    /** Observer methods called for the different stages of app installation. */
    public static interface Observer {
        /**
         * Called when the installation intent completes and installation has begun.
         * @param delegate     The delegate object sending the message.
         * @param isInstalling true if the app is currently installing.
         */
        public void onInstallIntentCompleted(InstallerDelegate delegate, boolean isInstalling);

        /**
         * Called when the task has finished.
         * @param delegate The delegate object sending the message.
         * @param success  Whether or not the package was successfully installed.
         */
        public void onInstallFinished(InstallerDelegate delegate, boolean success);

        /**
         * Called when the current application state changes due to an installation.
         * @param delegate The delegate object sending the message.
         * @param newState the new state id.
         */
        public void onApplicationStateChanged(InstallerDelegate delegate, int newState);
    }

    /**
     * Object to wait for the PackageManager to finish installation an app. For convenience, this is
     * bound to an instance of InstallerDelegate, and accesses its members and methods.
     */
    private class InstallMonitor implements Runnable {
        /** Timestamp of when we started monitoring. */
        private long mTimestampStarted;

        public InstallMonitor() {}

        /** Begin monitoring the PackageManager to see if it completes installing the package. */
        public void start() {
            mTimestampStarted = SystemClock.elapsedRealtime();
            mIsRunning = true;
            mHandler.postDelayed(this, mMsBetweenRuns);
        }

        /** Don't call this directly; instead, call {@link #start()}. */
        @Override
        public void run() {
            boolean isInstalled = InstallerDelegate.isInstalled(
                    getPackageManager(ContextUtils.getApplicationContext()), mPackageName);
            boolean waitedTooLong =
                    (SystemClock.elapsedRealtime() - mTimestampStarted) > mMsMaximumWaitingTime;
            if (isInstalled || !mIsRunning || waitedTooLong) {
                mObserver.onInstallFinished(InstallerDelegate.this, isInstalled);
                mIsRunning = false;
            } else {
                mHandler.postDelayed(this, mMsBetweenRuns);
            }
        }

        /** Prevent rescheduling the Runnable. */
        public void cancel() {
            mIsRunning = false;
        }
    }

    private static final String TAG = "cr_InstallerDelegate";
    private static final long DEFAULT_MS_BETWEEN_RUNS = 1000;
    private static final long DEFAULT_MS_MAXIMUM_WAITING_TIME = 3 * 60 * 1000;

    /** PackageManager to use in place of the real one. */
    private static PackageManager sPackageManagerForTests;

    /** Monitors an installation in progress. */
    private InstallMonitor mInstallMonitor;

    /** Message loop to post the Runnable to. */
    private final Handler mHandler;

    /** Monitors for application state changes. */
    private final ApplicationStatus.ApplicationStateListener mListener;

    /** The name of the package currently being installed. */
    private String mPackageName;

    /** Milliseconds to wait between calls to check on the PackageManager during installation. */
    private long mMsBetweenRuns;

    /** Maximum milliseconds to wait before giving up on monitoring during installation. */
    private long mMsMaximumWaitingTime;

    /** The observer to inform of updates during the installation process. */
    private Observer mObserver;

    /** Whether or we are currently monitoring an installation. */
    private boolean mIsRunning;

    /** Overrides the PackageManager for testing. */
    @VisibleForTesting
    static void setPackageManagerForTesting(PackageManager manager) {
        sPackageManagerForTests = manager;
    }

    /**
     * Set how often the handler will check the PackageManager.
     * @param msBetween How long to wait between executions of the Runnable.
     * @param msMax     How long to wait before giving up.
     */
    @VisibleForTesting
    void setTimingForTests(long msBetween, long msMax) {
        mMsBetweenRuns = msBetween;
        mMsMaximumWaitingTime = msMax;
    }

    /**
     * Checks if the app has been installed on the system.
     * @param packageManager PackageManager to use.
     * @param packageName    Name of the package to check.
     * @return True if the PackageManager reports that the app is installed, false otherwise.
     */
    public static boolean isInstalled(PackageManager packageManager, String packageName) {
        try {
            packageManager.getPackageInfo(packageName, 0);
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
        return true;
    }

    /**
     * Construct an InstallerDelegate to monitor an installation.
     * @param looper   Thread to run the monitor on.
     * @param observer Object to inform of changes in the installation state.
     */
    public InstallerDelegate(Looper looper, Observer observer) {
        mHandler = new Handler(looper);
        mObserver = observer;
        mListener = createApplicationStateListener();
        ApplicationStatus.registerApplicationStateListener(mListener);

        mMsBetweenRuns = DEFAULT_MS_BETWEEN_RUNS;
        mMsMaximumWaitingTime = DEFAULT_MS_MAXIMUM_WAITING_TIME;
    }

    /**
     * Stops all current monitoring.
     */
    public void destroy() {
        if (mInstallMonitor != null) {
            mInstallMonitor.cancel();
            mInstallMonitor = null;
        }
        ApplicationStatus.unregisterApplicationStateListener(mListener);
    }

    /**
     * Checks if the app has been installed on the system.
     * @param context     Context to use.
     * @param packageName Name of the package to check.
     * @return True if the PackageManager reports that the app is installed, false otherwise.
     */
    public boolean isInstalled(Context context, String packageName) {
        return isInstalled(getPackageManager(context), packageName);
    }

    /**
     * Checks to see if we are currently monitoring an installation.
     * @return true if we are currently monitoring, false otherwise.
     */
    public boolean isRunning() {
        return mIsRunning;
    }

    /**
     * Called to start monitoring an installation. Exposed to allow manual triggering in tests; this
     * is normally called when the installation intent returns.
     */
    void startMonitoring(String packageName) {
        mPackageName = packageName;
        mInstallMonitor = new InstallMonitor();
        mInstallMonitor.start();
    }

    /** Simulates a cancellation for testing. */
    @VisibleForTesting
    void cancel() {
        mInstallMonitor.cancel();
    }

    /**
     * Attempts to open the specified app in the given content. Returns true if opening was
     * successful, otherwise returns false if the app is not installed or opening failed.
     * @param context     The context to use
     * @param packageName Name of the package to open.
     */
    public boolean openApp(Context context, String packageName) {
        PackageManager packageManager = getPackageManager(context);
        if (!isInstalled(packageManager, packageName)) {
            return false;
        }

        Intent launchIntent = packageManager.getLaunchIntentForPackage(packageName);
        if (launchIntent != null) {
            try {
                context.startActivity(launchIntent);
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "Failed to open app : %s!", packageName, e);
                return false;
            }
        }
        return true;
    }

    /**
     * Attempts to install or open a native app specified by the given data.
     * @return true if the app was opened or if installation was started successfully. false
     * otherwise.
     * @param tab      The current tab.
     * @param appData  The native app data to try and open or install.
     * @param referrer The referrer attached to the URL specifying the native app, if any.
     */
    public boolean installOrOpenNativeApp(Tab tab, AppData appData, String referrer) {
        Context context = ContextUtils.getApplicationContext();
        String packageName = appData.packageName();
        PackageManager packageManager = getPackageManager(context);
        if (openApp(context, packageName)) {
            return true;
        } else {
            // Try installing the app.  If the installation was kicked off, return false to prevent
            // the infobar from disappearing.
            // The supplied referrer is the URL of the page requesting the native app banner. It
            // may be empty depending on that page's referrer policy. If it is non-empty, attach it
            // to the installation intent as Intent.EXTRA_REFERRER.
            Intent installIntent = appData.installIntent();
            if (referrer.length() > 0) installIntent.putExtra(Intent.EXTRA_REFERRER, referrer);
            return !tab.getWindowAndroid().showIntent(
                    installIntent, createIntentCallback(appData), null);
        }
    }

    private WindowAndroid.IntentCallback createIntentCallback(final AppData appData) {
        return new WindowAndroid.IntentCallback() {
            @Override
            public void onIntentCompleted(WindowAndroid window, int resultCode, Intent data) {
                boolean isInstalling = resultCode == Activity.RESULT_OK;
                if (isInstalling) {
                    startMonitoring(appData.packageName());
                }

                mObserver.onInstallIntentCompleted(InstallerDelegate.this, isInstalling);
            }
        };
    }

    private ApplicationStatus.ApplicationStateListener createApplicationStateListener() {
        return new ApplicationStatus.ApplicationStateListener() {
            @Override
            public void onApplicationStateChange(int newState) {
                if (!ApplicationStatus.hasVisibleActivities()) return;
                mObserver.onApplicationStateChanged(InstallerDelegate.this, newState);
            }
        };
    }

    private PackageManager getPackageManager(Context context) {
        if (sPackageManagerForTests != null) return sPackageManagerForTests;
        return context.getPackageManager();
    }
}
