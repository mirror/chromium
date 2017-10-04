// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;

import java.io.File;
import java.util.List;

abstract class HostBrowserLaunchingActivity extends Activity {
    // Action for launching {@link WebappLauncherActivity}.
    // TODO(hanxi): crbug.com/737556. Replaces this string with the new WebAPK launch action after
    // it is propagated to all the Chrome's channels.
    public static final String ACTION_START_WEBAPK =
            "com.google.android.apps.chrome.webapps.WebappManager.ACTION_START_WEBAPP";
    private static final String LAST_RESORT_HOST_BROWSER = "com.android.chrome";
    private static final String LAST_RESORT_HOST_BROWSER_APPLICATION_NAME = "Google Chrome";
    private static final String TAG = "cr_HostBrowserLaunchAct";

    /**
     * Creates install Intent.
     * @param packageName Package to install.
     * @return The intent.
     */
    private static Intent createInstallIntent(String packageName) {
        String marketUrl = "market://details?id=" + packageName;
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(marketUrl));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    /** Returns whether there is any installed browser supporting WebAPKs. */
    private static boolean hasBrowserSupportingWebApks(List<ResolveInfo> resolveInfos) {
        List<String> browsersSupportingWebApk = WebApkUtils.getBrowsersSupportingWebApk();
        for (ResolveInfo info : resolveInfos) {
            if (browsersSupportingWebApk.contains(info.activityInfo.packageName)) {
                return true;
            }
        }
        return false;
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Bundle metadata = WebApkUtils.readMetaData(this);
        if (metadata == null || !canHandleIntent(savedInstanceState)) {
            finish();
            return;
        }

        String packageName = getPackageName();
        Log.v(TAG, "Package name of the WebAPK:" + packageName);

        String runtimeHostInPreferences = WebApkUtils.getHostBrowserFromSharedPreference(this);
        String runtimeHost = WebApkUtils.getHostBrowserPackageName(this);
        if (!TextUtils.isEmpty(runtimeHostInPreferences)
                && !runtimeHostInPreferences.equals(runtimeHost)) {
            deleteSharedPref(this);
            deleteInternalStorage();
        }

        if (!TextUtils.isEmpty(runtimeHost)) {
            launchInHostBrowser(runtimeHost);
            finish();
            return;
        }

        List<ResolveInfo> infos = WebApkUtils.getInstalledBrowserResolveInfos(getPackageManager());
        if (hasBrowserSupportingWebApks(infos)) {
            showChooseHostBrowserDialog(infos);
        } else {
            showInstallHostBrowserDialog(metadata);
        }
    }

    /** Deletes the SharedPreferences. */
    private void deleteSharedPref(Context context) {
        SharedPreferences sharedPref =
                context.getSharedPreferences(WebApkConstants.PREF_PACKAGE, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.clear();
        editor.apply();
    }

    /** Deletes the internal storage. */
    private void deleteInternalStorage() {
        deletePath(getCacheDir());
        deletePath(getFilesDir());
        deletePath(getDir(HostBrowserClassLoader.DEX_DIR_NAME, Context.MODE_PRIVATE));
    }

    private void deletePath(File file) {
        if (file == null) return;

        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    deletePath(child);
                }
            }
        }

        if (!file.delete()) {
            Log.e(TAG, "Failed to delete : " + file.getAbsolutePath());
        }
    }

    private void launchInHostBrowser(String runtimeHost) {
        try {
            Intent intent = getLaunchIntent();
            intent.setPackage(runtimeHost);
            startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.w(TAG, "Unable to launch browser in WebAPK mode.");
            e.printStackTrace();
        }
    }

    /**
     * Launches the Play Store with the host browser's page.
     */
    private void installBrowser(String hostBrowserPackageName) {
        try {
            startActivity(createInstallIntent(hostBrowserPackageName));
        } catch (ActivityNotFoundException e) {
        }
    }

    /** Shows a dialog to choose the host browser. */
    private void showChooseHostBrowserDialog(List<ResolveInfo> infos) {
        ChooseHostBrowserDialog.DialogListener listener =
                new ChooseHostBrowserDialog.DialogListener() {
                    @Override
                    public void onHostBrowserSelected(String selectedHostBrowser) {
                        launchInHostBrowser(selectedHostBrowser);
                        WebApkUtils.writeHostBrowserToSharedPref(
                                HostBrowserLaunchingActivity.this, selectedHostBrowser);
                        finish();
                    }
                    @Override
                    public void onQuit() {
                        finish();
                    }
                };
        ChooseHostBrowserDialog.show(this, listener, infos, getString(R.string.name));
    }

    /** Shows a dialog to install the host browser. */
    private void showInstallHostBrowserDialog(Bundle metadata) {
        String lastResortHostBrowserPackageName =
                metadata.getString(WebApkMetaDataKeys.RUNTIME_HOST);
        String lastResortHostBrowserApplicationName =
                metadata.getString(WebApkMetaDataKeys.RUNTIME_HOST_APPLICATION_NAME);

        if (TextUtils.isEmpty(lastResortHostBrowserPackageName)) {
            // WebAPKs without runtime host specified in the AndroidManifest.xml always install
            // Google Chrome as the default host browser.
            lastResortHostBrowserPackageName = LAST_RESORT_HOST_BROWSER;
            lastResortHostBrowserApplicationName = LAST_RESORT_HOST_BROWSER_APPLICATION_NAME;
        }

        InstallHostBrowserDialog.DialogListener listener =
                new InstallHostBrowserDialog.DialogListener() {
                    @Override
                    public void onConfirmInstall(String packageName) {
                        installBrowser(packageName);
                        WebApkUtils.writeHostBrowserToSharedPref(
                                HostBrowserLaunchingActivity.this, packageName);
                        finish();
                    }
                    @Override
                    public void onConfirmQuit() {
                        finish();
                    }
                };

        InstallHostBrowserDialog.show(this, listener, getString(R.string.name),
                lastResortHostBrowserPackageName, lastResortHostBrowserApplicationName,
                R.drawable.last_resort_runtime_host_logo);
    }

    /**
     * Implements validation that all preconditions are met for handling the intent.
     * @param savedInstanceState The savedInstanceState to be used if needed for restoring activity
     * state.
     * @return whether the intent can be handled. A false value will result in the activity being
     * finished and the event being swallowed.
     */
    protected abstract boolean canHandleIntent(Bundle savedInstanceState);

    /**
     * @return returns the specialized intent used for launching the host browser.
     */
    protected abstract Intent getLaunchIntent();
}
