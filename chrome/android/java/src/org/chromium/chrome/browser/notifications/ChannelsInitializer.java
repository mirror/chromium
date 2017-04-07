// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.app.NotificationManager;
import android.support.annotation.StringDef;

import org.chromium.base.ContextUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.Map;

/**
 * Initializes our notification channels.
 */
public class ChannelsInitializer {
    // To define a new channel, add the channel ID to this StringDef and add a new entry to the
    // sChannels map below with the appropriate channel parameters.
    @StringDef({CHANNEL_ID_BROWSER, CHANNEL_ID_SITES})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ChannelId {}
    public static final String CHANNEL_ID_BROWSER = "browser";
    public static final String CHANNEL_ID_SITES = "sites";

    @StringDef({CHANNEL_GROUP_ID_GENERAL})
    @Retention(RetentionPolicy.SOURCE)
    private @interface ChannelGroupId {}
    static final String CHANNEL_GROUP_ID_GENERAL = "general";

    private static final Map<String, NotificationChannel> sChannels = new HashMap<>();
    static {
        sChannels.put(CHANNEL_ID_BROWSER,
                new NotificationChannel(CHANNEL_ID_BROWSER,
                        ContextUtils.getApplicationContext().getString(
                                org.chromium.chrome.R.string.notification_category_browser),
                        NotificationManager.IMPORTANCE_LOW, CHANNEL_GROUP_ID_GENERAL));
        sChannels.put(CHANNEL_ID_SITES,
                new NotificationChannel(CHANNEL_ID_SITES,
                        ContextUtils.getApplicationContext().getString(
                                org.chromium.chrome.R.string.notification_category_sites),
                        NotificationManager.IMPORTANCE_DEFAULT, CHANNEL_GROUP_ID_GENERAL));
    }

    private static final ChannelGroup GENERAL_CHANNEL_GROUP =
            new ChannelGroup(CHANNEL_GROUP_ID_GENERAL,
                    ContextUtils.getApplicationContext().getString(
                            org.chromium.chrome.R.string.notification_category_group_general));

    private final NotificationManagerProxy mNotificationManager;

    public ChannelsInitializer(NotificationManagerProxy notificationManagerProxy) {
        mNotificationManager = notificationManagerProxy;
    }

    /**
     * Ensures the given channel has been created on the notification manager so a notification
     * can be safely posted to it. This should only be used for channels that are predefined in
     * ChannelsInitializer.
     *
     * Calling this is a (potentially lengthy) no-op if the channel has already been created.
     *
     * @param channelId The ID of the channel to be initialized.
     */
    public void ensureInitialized(@ChannelId String channelId) {
        createNotificationChannel(sChannels.get(channelId), GENERAL_CHANNEL_GROUP);
    }

    private void createNotificationChannel(NotificationChannel channel, ChannelGroup channelGroup) {
        mNotificationManager.createNotificationChannelGroup(channelGroup);
        mNotificationManager.createNotificationChannel(channel);
    }

    /**
     * Helper class containing notification channel properties until we can use the real
     * NotificationChannel class once compileSdkVersion is bumped to O.
     */
    public static class NotificationChannel {
        final String mId;
        final String mName;
        final int mImportance;
        final String mGroupId;

        NotificationChannel(@ChannelId String id, String name, int importance, String groupId) {
            this.mId = id;
            this.mName = name;
            this.mImportance = importance;
            this.mGroupId = groupId;
        }
    }

    /**
     * Helper class containing notification channel group properties until we can use the real
     * NotificationChannelGroup class once compileSdkVersion is bumped to O.
     */
    public static class ChannelGroup {
        final String mId;
        final String mName;

        ChannelGroup(@ChannelGroupId String id, String name) {
            this.mId = id;
            this.mName = name;
        }
    }
}
