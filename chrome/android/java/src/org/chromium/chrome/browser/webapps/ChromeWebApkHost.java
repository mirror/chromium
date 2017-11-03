// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import static org.chromium.chrome.browser.ChromeSwitches.SKIP_WEBAPK_VERIFICATION;

import android.Manifest;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.text.TextUtils;

import org.chromium.base.ApplicationState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.chrome.browser.ChromeVersionInfo;
import org.chromium.webapk.lib.client.WebApkIdentityServiceClient;
import org.chromium.webapk.lib.client.WebApkServiceImplClient;
import org.chromium.webapk.lib.client.WebApkValidator;

/**
 * Contains functionality needed for Chrome to host WebAPKs.
 */
public class ChromeWebApkHost {
    private static ApplicationStatus.ApplicationStateListener sListener;

    public static void init() {
        WebApkValidator.init(ChromeWebApkHostSignature.EXPECTED_SIGNATURE,
                ChromeWebApkHostSignature.PUBLIC_KEY);
        if (ChromeVersionInfo.isLocalBuild()
                && CommandLine.getInstance().hasSwitch(SKIP_WEBAPK_VERIFICATION)) {
            // Tell the WebApkValidator to work for all WebAPKs.
            WebApkValidator.disableValidationForTesting();
        }
    }

    /* Returns whether launching renderer in WebAPK process is enabled by Chrome. */
    public static boolean canLaunchRendererInWebApkProcess() {
        return LibraryLoader.isInitialized() && nativeCanLaunchRendererInWebApkProcess();
    }

    /**
     * Checks whether Chrome is the runtime host of the WebAPK asynchronously. Accesses the
     * ApplicationStateListener needs to be called on UI thread.
     */
    @SuppressFBWarnings("LI_LAZY_INIT_UPDATE_STATIC")
    public static void checkChromeBacksWebApkAsync(String webApkPackageName,
            WebApkIdentityServiceClient.CheckBrowserBacksWebApkCallback callback) {
        ThreadUtils.assertOnUiThread();

        if (sListener == null) {
            // Registers an application listener to disconnect all connections to WebAPKs
            // when Chrome is stopped.
            sListener = new ApplicationStatus.ApplicationStateListener() {
                @Override
                public void onApplicationStateChange(int newState) {
                    if (newState == ApplicationState.HAS_STOPPED_ACTIVITIES
                            || newState == ApplicationState.HAS_DESTROYED_ACTIVITIES) {
                        WebApkIdentityServiceClient.disconnectAll(
                                ContextUtils.getApplicationContext());
                        WebApkServiceImplClient.disconnectAll(ContextUtils.getApplicationContext());

                        ApplicationStatus.unregisterApplicationStateListener(sListener);
                        sListener = null;
                    }
                }
            };
            ApplicationStatus.registerApplicationStateListener(sListener);
        }

        WebApkIdentityServiceClient.getInstance().checkBrowserBacksWebApkAsync(
                ContextUtils.getApplicationContext(), webApkPackageName, callback);
    }

    @CalledByNative
    private static boolean hasPermission(String packageName, String permission) {
        PackageInfo packageInfo;
        try {
            packageInfo = ContextUtils.getApplicationContext().getPackageManager().getPackageInfo(
                    packageName, PackageManager.GET_PERMISSIONS);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
            return false;
        }
        String[] requestedPermissions = packageInfo.requestedPermissions;
        if (requestedPermissions == null) return false;

        // Loops each <uses-permission> tag to retrieve the permission flag.
        for (int i = 0; i < requestedPermissions.length; ++i) {
            if (!TextUtils.equals(requestedPermissions[i], permission)) continue;

            if ((packageInfo.requestedPermissionsFlags[i]
                        & PackageInfo.REQUESTED_PERMISSION_GRANTED)
                    == 0) {
                return false;
            }
        }
        return true;
    }

    @CalledByNative
    private static boolean hasAndroidLocationPermission(String packageName) {
        return hasPermission(packageName, Manifest.permission.ACCESS_FINE_LOCATION)
                || hasPermission(packageName, Manifest.permission.ACCESS_COARSE_LOCATION);
    }

    @CalledByNative
    private static boolean isPermissionRevokedByPolicy(String packageName) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true;

        return ContextUtils.getApplicationContext().getPackageManager().isPermissionRevokedByPolicy(
                Manifest.permission.ACCESS_FINE_LOCATION, packageName);
    }

    private static native boolean nativeCanLaunchRendererInWebApkProcess();
}
