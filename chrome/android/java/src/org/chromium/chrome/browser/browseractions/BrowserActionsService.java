// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.NotificationConstants;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.IntentUtils;

/**
 * The foreground service responsible for creating background tabs and notifications for Browser
 * Actions.
 */
public class BrowserActionsService extends Service {
    public static final String ACTION_TAB_CREATION_START =
            "org.chromium.chrome.browser.browseractions.ACTION_TAB_CREATION_START";
    public static final String ACTION_TAB_CREATION_UPDATE =
            "org.chromium.chrome.browser.browseractions.ACTION_TAB_CREATION_UPDATE";
    public static final String ACTION_TAB_CREATION_FINISH =
            "org.chromium.chrome.browser.browseractions.ACTION_TAB_CREATION_FINISH";
    public static final String EXTRA_TAB_ID = "org.chromium.chrome.browser.browseractions.TAB_ID";

    /**
     * Action to request open ChromeTabbedActivity in tab switcher mode.
     */
    public static final String ACTION_BROWSER_ACTIONS_OPEN_IN_BACKGROUND =
            "org.chromium.chrome.browser.browseractions.browser_action_open_in_background";

    public static final String PREF_HAS_BROWSER_ACTIONS_NOTIFICATION =
            "org.chromium.chrome.browser.browseractions.HAS_BROWSER_ACTIONS_NOTIFICATION";

    /**
     * Extra that indicates whether to show a Tab for single url or the tab switcher for
     * multiple urls.
     */
    public static final String EXTRA_IS_SINGLE_URL =
            "org.chromium.chrome.browser.browseractions.is_single_url";

    private static Intent sNotificationIntent;

    private static int sTitleResId;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @VisibleForTesting
    static Intent getNotificationIntent() {
        return sNotificationIntent;
    }

    @VisibleForTesting
    static int getTitleRestId() {
        return sTitleResId;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (TextUtils.equals(intent.getAction(), ACTION_TAB_CREATION_START)) {
            sendBrowserActionsNotification(Tab.INVALID_TAB_ID);
            NotificationUmaTracker.getInstance().onNotificationShown(
                    NotificationUmaTracker.BROWSER_ACTIONS, ChannelDefinitions.CHANNEL_ID_BROWSER);
        } else if (TextUtils.equals(intent.getAction(), ACTION_TAB_CREATION_UPDATE)) {
            int tabId = IntentUtils.safeGetIntExtra(intent, EXTRA_TAB_ID, Tab.INVALID_TAB_ID);
            sendBrowserActionsNotification(tabId);
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .putBoolean(PREF_HAS_BROWSER_ACTIONS_NOTIFICATION, true)
                    .apply();
        } else if (TextUtils.equals(intent.getAction(), ACTION_TAB_CREATION_FINISH)) {
            stopForeground(false);
        }
        // The service will not be restarted if Chrome get killed.
        return START_NOT_STICKY;
    }

    private void sendBrowserActionsNotification(int tabId) {
        ChromeNotificationBuilder builder = createNotificationBuilder(tabId);
        startForeground(NotificationConstants.NOTIFICATION_ID_BROWSER_ACTIONS, builder.build());
    }

    private ChromeNotificationBuilder createNotificationBuilder(int tabId) {
        ChromeNotificationBuilder builder =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(
                                true /* preferCompat */, ChannelDefinitions.CHANNEL_ID_BROWSER)
                        .setSmallIcon(R.drawable.infobar_chrome)
                        .setLocalOnly(true)
                        .setAutoCancel(true)
                        .setContentText(this.getString(R.string.browser_actions_notification_text));
        sTitleResId = hasBrowserActionsNotification()
                ? R.string.browser_actions_multi_links_open_notification_title
                : R.string.browser_actions_single_link_open_notification_title;
        builder.setContentTitle(this.getString(sTitleResId));
        sNotificationIntent = buildNotificationIntent(tabId);
        PendingIntent notifyPendingIntent = PendingIntent.getActivity(
                this, 0, sNotificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);
        builder.setContentIntent(notifyPendingIntent);
        return builder;
    }

    private Intent buildNotificationIntent(int tabId) {
        boolean multipleUrls = hasBrowserActionsNotification();
        if (!multipleUrls && tabId != Tab.INVALID_TAB_ID) {
            return Tab.createBringTabToFrontIntent(tabId);
        }
        Intent intent = new Intent(this, ChromeLauncherActivity.class);
        intent.setAction(ACTION_BROWSER_ACTIONS_OPEN_IN_BACKGROUND);
        intent.putExtra(EXTRA_IS_SINGLE_URL, !multipleUrls);
        return intent;
    }

    @VisibleForTesting
    static boolean hasBrowserActionsNotification() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_HAS_BROWSER_ACTIONS_NOTIFICATION, false);
    }

    /**
     * Cancel Browser Actions notification.
     */
    public static void cancelBrowserActionsNotification() {
        NotificationManager notificationManager =
                (NotificationManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(NotificationConstants.NOTIFICATION_ID_BROWSER_ACTIONS);
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(PREF_HAS_BROWSER_ACTIONS_NOTIFICATION, false)
                .apply();
    }

    /**
     * Checks whether Chrome should display tab switcher via Browser Actions Intent.
     * @param intent The intent to open the Chrome.
     * @param isOverviewVisible Whether tab switcher is shown.
     */
    public static boolean shouldToggleOverview(Intent intent, boolean isOverviewVisible) {
        boolean fromBrowserActions = isStartedByBrowserActions(intent);
        boolean isSingleUrl = IntentUtils.safeGetBooleanExtra(intent, EXTRA_IS_SINGLE_URL, false);
        if (fromBrowserActions) {
            return isSingleUrl == isOverviewVisible;
        }
        return false;
    }

    private static boolean isStartedByBrowserActions(Intent intent) {
        if (ACTION_BROWSER_ACTIONS_OPEN_IN_BACKGROUND.equals(intent.getAction())) {
            return true;
        }
        return false;
    }

    /**
     * Sends a {@link Intent} to update the notification and the status of the {@link
     * BrowserActionsService}.
     * @param action The actions of the Intent.
     * @param tabId The id of the created Tab.
     */
    public static void sendIntent(String action, int tabId) {
        Context context = ContextUtils.getApplicationContext();
        Intent intent = new Intent(context, BrowserActionsService.class);
        intent.setAction(action);
        if (tabId != Tab.INVALID_TAB_ID) {
            intent.putExtra(EXTRA_TAB_ID, tabId);
        }
        context.startService(intent);
    }
}
