// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.os.RemoteException;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.notifications.NotificationBuilderBase;
import org.chromium.webapk.lib.client.WebApkServiceImplClient;
import org.chromium.webapk.lib.runtime_library.IWebApkApi;

/**
 * Provides APIs for browsers to communicate with WebAPK services. Each WebAPK has its own "WebAPK
 * service".
 */
public class WebApkServiceClient {
    /**
     * Connects to a WebAPK's bound service, builds a notification and hands it over to the WebAPK
     * to display. Handing over the notification makes the notification look like it originated from
     * the WebAPK - not Chrome - in the Android UI.
     */
    public static void notifyNotification(final String webApkPackage,
            final NotificationBuilderBase notificationBuilder, final String platformTag,
            final int platformID) {
        WebApkServiceImplClient.ConnectionCallback callback =
                new WebApkServiceImplClient.ConnectionCallback() {
                    @Override
                    public void onConnected(IWebApkApi api) {
                        try {
                            int smallIconId = api.getSmallIconId();
                            // Prior to Android M, the small icon had to be from the resources of
                            // the app whose NotificationManager is used in {@link
                            // NotificationManager#notify()}. On Android M+, the small icon has to
                            // be from the resources of the app whose context is passed to the
                            // {@link Notification.Builder()} constructor.
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                                // Android M+ introduced
                                // {@link Notification.Builder#setSmallIcon(Bitmap icon)}.
                                if (!notificationBuilder.hasSmallIconBitmap()) {
                                    notificationBuilder.setSmallIcon(
                                            decodeImageResource(webApkPackage, smallIconId));
                                }
                            } else {
                                notificationBuilder.setSmallIcon(smallIconId);
                            }
                            api.notifyNotification(
                                    platformTag, platformID, notificationBuilder.build());
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                };
        WebApkServiceImplClient.getInstance().notifyNotification(
                webApkPackage, platformTag, platformID, callback);
    }

    /** Cancels notification previously shown by WebAPK. */
    public static void cancelNotification(
            String webApkPackage, String platformTag, int platformID) {
        WebApkServiceImplClient.getInstance().cancelNotification(
                webApkPackage, platformTag, platformID);
    }

    /** Decodes bitmap from WebAPK's resources. */
    private static Bitmap decodeImageResource(String webApkPackage, int resourceId) {
        PackageManager packageManager = ContextUtils.getApplicationContext().getPackageManager();
        try {
            Resources resources = packageManager.getResourcesForApplication(webApkPackage);
            return BitmapFactory.decodeResource(resources, resourceId);
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
    }
}
