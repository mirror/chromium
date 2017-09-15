// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages.prefetch;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import org.chromium.base.BuildInfo;
import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.browser.preferences.NotificationsPreferences;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content.browser.BrowserStartupController.StartupCallback;

/**
 * Helper that can notify about prefetched pages and also receive the click events.
 */
public class PrefetchedPagesNotifier {
    private static final String NOTIFICATION_TAG = "OfflineContentSuggestionsNotification";

    // UMA histogram values tracking user actions on the prefetch histogram.
    public static final int NOTIFICATION_ACTION_MAY_SHOW = 0;
    public static final int NOTIFICATION_ACTION_SHOWN = 1;
    public static final int NOTIFICATION_ACTION_CLICKED = 2;
    public static final int NOTIFICATION_ACTION_SETTINGS_CLICKED = 3;

    public static final int NOTIFICATION_ACTION_COUNT = 4;

    /**
     * Opens the Downloads Home when the notification is tapped.
     */
    public static class ClickReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(final Context context, Intent intent) {
            // TODO(dewittj): Focus the download manager on prefetched content.
            // TODO(dewittj): Do something if download manager is not available.
            recordNotificationActionWhenChromeStarts(NOTIFICATION_ACTION_CLICKED);
            DownloadUtils.showDownloadManager(null, null);
        }
    }

    /**
     * Opens notification settings when the settings button is tapped.  This should never be called
     * on O+ since notification settings are provided by the OS.
     */
    public static class SettingsReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(final Context context, Intent intent) {
            recordNotificationActionWhenChromeStarts(NOTIFICATION_ACTION_SETTINGS_CLICKED);
            Intent settingsIntent = PreferencesLauncher.createIntentForSettingsPage(
                    context, NotificationsPreferences.class.getName());
            settingsIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
            context.startActivity(settingsIntent);
        }
    }

    /**
     * Shows the prefetching notification.
     * @param origin A string representing the origin of a relevant prefetched page.
     */
    @CalledByNative
    public static void showNotification(String origin) {
        Context context = ContextUtils.getApplicationContext();

        // TODO(dewittj): Use unique notification IDs, allowing multiple to appear in the
        // notification center.
        int notificationId = 1;
        PendingIntent clickIntent =
                PendingIntent.getBroadcast(context, 0, new Intent(context, ClickReceiver.class), 0);

        String title =
                String.format(context.getString(R.string.offline_pages_prefetch_notification_title),
                        context.getString(R.string.app_name));
        String text = String.format(
                context.getString(R.string.offline_pages_prefetch_notification_text), origin);

        ChromeNotificationBuilder builder =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(true /* preferCompat */,
                                ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS)
                        .setAutoCancel(true)
                        .setContentIntent(clickIntent)
                        .setContentTitle(title)
                        .setContentText(text)
                        .setGroup(NOTIFICATION_TAG)
                        .setPriority(Notification.PRIORITY_LOW)
                        .setSmallIcon(R.drawable.ic_chrome);
        if (!BuildInfo.isAtLeastO()) {
            PendingIntent settingsIntent = PendingIntent.getBroadcast(
                    context, 0, new Intent(context, SettingsReceiver.class), 0);
            builder.addAction(R.drawable.settings_cog, context.getString(R.string.preferences),
                    settingsIntent);
        }

        NotificationManager manager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        manager.notify(NOTIFICATION_TAG, notificationId, builder.build());

        // Metrics tracking
        recordNotificationActionWhenChromeStarts(NOTIFICATION_ACTION_SHOWN);
        NotificationUmaTracker.getInstance().onNotificationShown(
                NotificationUmaTracker.OFFLINE_CONTENT_SUGGESTION,
                ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS);
    }

    private static void recordNotificationActionWhenChromeStarts(final int action) {
        runWhenChromeStarts(new Runnable() {
            @Override
            public void run() {
                RecordHistogram.recordEnumeratedHistogram(
                        "OfflinePages.Prefetching.NotificationAction", action,
                        NOTIFICATION_ACTION_COUNT);
            }
        });
    }

    private static void runWhenChromeStarts(final Runnable r) {
        BrowserStartupController browserStartup =
                BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER);
        if (!browserStartup.isStartupSuccessfullyCompleted()) {
            browserStartup.addStartupCompletedObserver(new StartupCallback() {
                @Override
                public void onSuccess(boolean alreadyStarted) {
                    r.run();
                }
                @Override
                public void onFailure() {}
            });
            return;
        }

        r.run();
    }
}
