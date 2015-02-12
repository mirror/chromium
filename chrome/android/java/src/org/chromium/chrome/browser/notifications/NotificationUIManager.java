// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.Preferences;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.chrome.browser.preferences.website.WebsitePreferences;
import org.chromium.chrome.browser.widget.RoundedIconGenerator;

/**
 * Provides the ability for the NotificationUIManagerAndroid to talk to the Android platform
 * notification manager.
 *
 * This class should only be used on the UI thread.
 */
public class NotificationUIManager {
    private static final String TAG = NotificationUIManager.class.getSimpleName();

    // Category in the preferences for displaying Push Notification permissions.
    private static final String CATEGORY_PUSH_NOTIFICATIONS = "push_notifications";

    private static final int NOTIFICATION_ICON_BG_COLOR = Color.rgb(150, 150, 150);
    private static final int NOTIFICATION_TEXT_SIZE_DP = 28;

    private static NotificationUIManager sInstance;

    private final long mNativeNotificationManager;

    private final Context mAppContext;
    private final NotificationManager mNotificationManager;

    private RoundedIconGenerator mIconGenerator;

    private int mLastNotificationId;

    /**
     * Creates a new instance of the NotificationUIManager.
     *
     * @param nativeNotificationManager Instance of the NotificationUIManagerAndroid class.
     * @param context Application context for this instance of Chrome.
     */
    @CalledByNative
    private static NotificationUIManager create(long nativeNotificationManager, Context context) {
        if (sInstance != null) {
            throw new IllegalStateException("There must only be a single NotificationUIManager.");
        }

        sInstance = new NotificationUIManager(nativeNotificationManager, context);
        return sInstance;
    }

    private NotificationUIManager(long nativeNotificationManager, Context context) {
        mNativeNotificationManager = nativeNotificationManager;
        mAppContext = context.getApplicationContext();

        mNotificationManager = (NotificationManager)
                mAppContext.getSystemService(Context.NOTIFICATION_SERVICE);

        mLastNotificationId = 0;
    }

    /**
     * Marks the current instance as being freed, allowing for a new NotificationUIManager
     * object to be initialized.
     */
    @CalledByNative
    private void destroy() {
        assert sInstance == this;
        sInstance = null;
    }

    /**
     * Invoked by the NotificationService when a Notification intent has been received. There may
     * not be an active instance of the NotificationUIManager at this time, so inform the native
     * side through a static method, initialzing the manager if needed.
     *
     * @param intent The intent as received by the Notification service.
     * @return Whether the event could be handled by the native Notification manager.
     */
    public static boolean dispatchNotificationEvent(Intent intent) {
        if (sInstance == null) {
            nativeInitializeNotificationUIManager();
            if (sInstance == null) {
                Log.e(TAG, "Unable to initialize the native NotificationUIManager.");
                return false;
            }
        }

        String notificationId = intent.getStringExtra(NotificationConstants.EXTRA_NOTIFICATION_ID);
        if (NotificationConstants.ACTION_CLICK_NOTIFICATION.equals(intent.getAction())) {
            if (!intent.hasExtra(NotificationConstants.EXTRA_NOTIFICATION_DATA)
                    || !intent.hasExtra(NotificationConstants.EXTRA_NOTIFICATION_PLATFORM_ID)) {
                Log.e(TAG, "Not all required notification data has been set in the intent.");
                return false;
            }

            int platformId =
                    intent.getIntExtra(NotificationConstants.EXTRA_NOTIFICATION_PLATFORM_ID, -1);
            byte[] notificationData =
                    intent.getByteArrayExtra(NotificationConstants.EXTRA_NOTIFICATION_DATA);
            return sInstance.onNotificationClicked(notificationId, platformId, notificationData);
        }

        if (NotificationConstants.ACTION_CLOSE_NOTIFICATION.equals(intent.getAction())) {
            return sInstance.onNotificationClosed(notificationId);
        }

        Log.e(TAG, "Unrecognized Notification action: " + intent.getAction());
        return false;
    }

    private PendingIntent getPendingIntent(String action, String notificationId, int platformId,
                                           byte[] notificationData) {
        Intent intent = new Intent(action);
        intent.setClass(mAppContext, NotificationService.Receiver.class);
        intent.putExtra(NotificationConstants.EXTRA_NOTIFICATION_ID, notificationId);
        intent.putExtra(NotificationConstants.EXTRA_NOTIFICATION_PLATFORM_ID, platformId);
        intent.putExtra(NotificationConstants.EXTRA_NOTIFICATION_DATA, notificationData);

        return PendingIntent.getBroadcast(mAppContext, platformId, intent,
                PendingIntent.FLAG_UPDATE_CURRENT);
    }

    /**
     * Displays a notification with the given |notificationId|, |title|, |body| and |icon|.
     *
     * @param notificationId Unique id provided by the Chrome Notification system.
     * @param title Title to be displayed in the notification.
     * @param body Message to be displayed in the notification. Will be trimmed to one line of
     *             text by the Android notification system.
     * @param icon Icon to be displayed in the notification. When this isn't a valid Bitmap, a
     *             default icon will be generated instead.
     * @param origin Full text of the origin, including the protocol, owning this notification.
     * @param notificationData Serialized data associated with the notification.
     * @return The id using which the notification can be identified.
     */
    @CalledByNative
    private int displayNotification(String notificationId, String title, String body, Bitmap icon,
                                    String origin, byte[] notificationData) {
        if (icon == null || icon.getWidth() == 0) {
            icon = getIconGenerator().generateIconForUrl(origin, true);
        }

        Resources res = mAppContext.getResources();

        // TODO(peter): The current implementation introduces a [Site settings] button for opening
        // the "Notifications" panel in the site settings section, rather than the settings of an
        // individual website, which it should do instead.

        Intent settingsIntent = PreferencesLauncher.createIntentForSettingsPage(mAppContext,
                WebsitePreferences.class.getName());

        Bundle arguments = new Bundle();
        arguments.putString(WebsitePreferences.EXTRA_CATEGORY, CATEGORY_PUSH_NOTIFICATIONS);
        arguments.putString(WebsitePreferences.EXTRA_TITLE,
                res.getString(R.string.push_notifications_permission_title));

        settingsIntent.putExtra(Preferences.EXTRA_SHOW_FRAGMENT_ARGUMENTS, arguments);
        PendingIntent pendingSettingsIntent = PendingIntent.getActivity(
                mAppContext, 0, settingsIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        NotificationCompat.Builder notificationBuilder = new NotificationCompat.Builder(mAppContext)
                .setContentTitle(title)
                .setContentText(body)
                .setStyle(new NotificationCompat.BigTextStyle().bigText(body))
                .setLargeIcon(icon)
                .setSmallIcon(R.drawable.notification_badge)
                .setContentIntent(getPendingIntent(
                        NotificationConstants.ACTION_CLICK_NOTIFICATION,
                        notificationId, mLastNotificationId, notificationData))
                .setDeleteIntent(getPendingIntent(
                        NotificationConstants.ACTION_CLOSE_NOTIFICATION,
                        notificationId, mLastNotificationId, notificationData))
                .addAction(R.drawable.settings_cog,
                           res.getString(R.string.page_info_site_settings_button),
                           pendingSettingsIntent)
                .setSubText(origin);

        mNotificationManager.notify(mLastNotificationId, notificationBuilder.build());

        return mLastNotificationId++;
    }

    /**
     * Ensures the existance of an icon generator, which is created lazily.
     *
     * @return The icon generator which can be used.
     */
    private RoundedIconGenerator getIconGenerator() {
        if (mIconGenerator == null) {
            Resources res = mAppContext.getResources();
            float density = res.getDisplayMetrics().density;

            int widthPx = res.getDimensionPixelSize(android.R.dimen.notification_large_icon_width);
            int heightPx =
                    res.getDimensionPixelSize(android.R.dimen.notification_large_icon_height);

            mIconGenerator = new RoundedIconGenerator(
                    mAppContext,
                    (int) (widthPx / density),
                    (int) (heightPx / density),
                    (int) (Math.min(widthPx, heightPx) / density / 2),
                    NOTIFICATION_ICON_BG_COLOR,
                    NOTIFICATION_TEXT_SIZE_DP);
        }

        return mIconGenerator;
    }

    /**
     * Closes the notification identified by |platformId|.
     */
    @CalledByNative
    private void closeNotification(int platformId) {
        mNotificationManager.cancel(platformId);
    }

    private boolean onNotificationClicked(String notificationId, int platformId,
                                          byte[] notificationData) {
        return nativeOnNotificationClicked(
                mNativeNotificationManager, notificationId, platformId, notificationData);
    }

    private boolean onNotificationClosed(String notificationId) {
        return nativeOnNotificationClosed(mNativeNotificationManager, notificationId);
    }

    private static native void nativeInitializeNotificationUIManager();

    private native boolean nativeOnNotificationClicked(
            long nativeNotificationUIManagerAndroid, String notificationId,
            int platformId, byte[] notificationData);
    private native boolean nativeOnNotificationClosed(
            long nativeNotificationUIManagerAndroid, String notificationId);
}
