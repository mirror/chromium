// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import static android.content.Context.NOTIFICATION_SERVICE;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.banners.InstallerDelegate;
import org.chromium.chrome.browser.metrics.WebApkUma;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.webapk.lib.client.WebApkNavigationClient;
import org.chromium.webapk.lib.common.WebApkConstants;

/**
 * Java counterpart to webapk_installer.h
 * Contains functionality to install / update WebAPKs.
 * This Java object is created by and owned by the native WebApkInstaller.
 */
public class WebApkInstaller {
    private static final String TAG = "WebApkInstaller";

    // Prefix used for generating a unique notification tag.
    private static final String WEBAPK_INSTALLER_NOTIFICATION_TAG_PREFIX =
            "webapk_installer_notification_tag_prefix.";

    /** We always use the same platform id for notifications. */
    private static final int PLATFORM_ID = -1;

    /** Weak pointer to the native WebApkInstaller. */
    private long mNativePointer;

    /** Talks to Google Play to install WebAPKs. */
    private final GooglePlayWebApkInstallDelegate mInstallDelegate;

    private WebApkInstaller(long nativePtr) {
        mNativePointer = nativePtr;
        mInstallDelegate = AppHooks.get().getGooglePlayWebApkInstallDelegate();
    }

    @CalledByNative
    private static WebApkInstaller create(long nativePtr) {
        return new WebApkInstaller(nativePtr);
    }

    @CalledByNative
    private void destroy() {
        mNativePointer = 0;
    }

    /**
     * Installs a WebAPK and monitors the installation.
     * @param packageName The package name of the WebAPK to install.
     * @param manifestUrl The Web Manifest URL of the WebAPK to install.
     * @param version The version of WebAPK to install.
     * @param title The title of the WebAPK to display during installation.
     * @param token The token from WebAPK Server.
     * @param url The start URL of the WebAPK to install.
     * @param source The source (either app banner or menu) that the install of a WebAPK was
     *               triggered.
     * @param icon The primary icon of the WebAPK to install.
     */
    @CalledByNative
    private void installWebApkAsync(final String packageName, final String manifestUrl, int version,
            final String title, String token, final String url, final int source,
            final Bitmap icon) {
        // Check whether the WebAPK package is already installed. The WebAPK may have been installed
        // by another Chrome version (e.g. Chrome Dev). We have to do this check because the Play
        // install API fails silently if the package is already installed.
        if (isWebApkInstalled(packageName)) {
            notify(WebApkInstallResult.SUCCESS, packageName, manifestUrl, title, url, icon);
            return;
        }

        if (mInstallDelegate == null) {
            notify(WebApkInstallResult.FAILURE, packageName, manifestUrl, title, url, icon);
            WebApkUma.recordGooglePlayInstallResult(
                    WebApkUma.GOOGLE_PLAY_INSTALL_FAILED_NO_DELEGATE);
            return;
        }

        Callback<Integer> callback = new Callback<Integer>() {
            @Override
            public void onResult(Integer result) {
                WebApkInstaller.this.notify(result, packageName, manifestUrl, title, url, icon);
                if (result == WebApkInstallResult.FAILURE) return;

                // Stores the source info of WebAPK in WebappDataStorage.
                WebappRegistry.getInstance().register(
                        WebApkConstants.WEBAPK_ID_PREFIX + packageName,
                        new WebappRegistry.FetchWebappDataStorageCallback() {
                            @Override
                            public void onWebappDataStorageRetrieved(WebappDataStorage storage) {
                                storage.updateSource(source);
                                storage.updateTimeOfLastCheckForUpdatedWebManifest();
                            }
                        });
            }
        };
        mInstallDelegate.installAsync(packageName, version, title, token, url, callback);
    }

    /** Notifies the caller with notification for installing a WebAPK. */
    private void notify(@WebApkInstallResult int result, String packageName, String manifestUrl,
            String title, String url, Bitmap icon) {
        if (result == WebApkInstallResult.SUCCESS) {
            showInstalledNotification(packageName, manifestUrl, title, url, icon);
        }
        notify(result);
    }

    private void notify(@WebApkInstallResult int result) {
        if (mNativePointer != 0) {
            nativeOnInstallFinished(mNativePointer, result);
        }
    }

    /** Displays a notification when a WebAPK is successfully installed. */
    private void showInstalledNotification(
            String webApkPackage, String manifestUrl, String title, String url, Bitmap icon) {
        Context context = ContextUtils.getApplicationContext();
        Intent intent = WebApkNavigationClient.createLaunchWebApkIntent(webApkPackage, url, false);
        PendingIntent clickPendingIntent =
                PendingIntent.getActivity(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);

        showNotification(manifestUrl, title, url, icon,
                context.getResources().getString(R.string.notification_webapk_installed),
                clickPendingIntent);
    }

    /** Display a notification when an install starts with the WebApk's package name ready. */
    @CalledByNative
    private void showInstallStartNotification(
            String manifestUrl, String title, String url, Bitmap icon) {
        showNotification(manifestUrl, title, url, icon,
                ContextUtils.getApplicationContext().getResources().getString(
                        R.string.notification_webapk_install_start, title),
                null);
    }

    private void showNotification(String notificationId, String title, String url, Bitmap icon,
            String message, PendingIntent clickPendingIntent) {
        Context context = ContextUtils.getApplicationContext();
        ChromeNotificationBuilder notificationBuilder =
                NotificationBuilderFactory.createChromeNotificationBuilder(
                        false /* preferCompat */, ChannelDefinitions.CHANNEL_ID_BROWSER);
        notificationBuilder.setContentTitle(title)
                .setContentText(message)
                .setLargeIcon(icon)
                .setSmallIcon(R.drawable.ic_chrome)
                .setContentIntent(clickPendingIntent)
                .setWhen(System.currentTimeMillis())
                .setSubText(UrlFormatter.formatUrlForSecurityDisplay(url, false /* showScheme */))
                .setAutoCancel(true);

        NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(NOTIFICATION_SERVICE);
        notificationManager.notify(WEBAPK_INSTALLER_NOTIFICATION_TAG_PREFIX + notificationId,
                PLATFORM_ID, notificationBuilder.build());
    }

    /** Cancels any ongoing notification for the WebAPK. */
    @CalledByNative
    private void cancelNotification(String notificationId) {
        NotificationManager notificationManager =
                (NotificationManager) ContextUtils.getApplicationContext().getSystemService(
                        NOTIFICATION_SERVICE);
        notificationManager.cancel(
                WEBAPK_INSTALLER_NOTIFICATION_TAG_PREFIX + notificationId, PLATFORM_ID);
    }

    /**
     * Updates a WebAPK installation.
     * @param packageName The package name of the WebAPK to install.
     * @param version The version of WebAPK to install.
     * @param title The title of the WebAPK to display during installation.
     * @param token The token from WebAPK Server.
     * @param url The start URL of the WebAPK to install.
     */
    @CalledByNative
    private void updateAsync(
            String packageName, int version, String title, String token, String url) {
        if (mInstallDelegate == null) {
            notify(WebApkInstallResult.FAILURE);
            return;
        }

        Callback<Integer> callback = new Callback<Integer>() {
            @Override
            public void onResult(Integer result) {
                WebApkInstaller.this.notify(result);
            }
        };
        mInstallDelegate.updateAsync(packageName, version, title, token, url, callback);
    }

    private boolean isWebApkInstalled(String packageName) {
        PackageManager packageManager = ContextUtils.getApplicationContext().getPackageManager();
        return InstallerDelegate.isInstalled(packageManager, packageName);
    }

    private native void nativeOnInstallFinished(
            long nativeWebApkInstaller, @WebApkInstallResult int result);
}
