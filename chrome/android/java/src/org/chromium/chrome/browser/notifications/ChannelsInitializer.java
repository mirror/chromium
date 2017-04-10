// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.app.NotificationManager;
import android.support.annotation.StringDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Initializes our notification channels.
 */
public class ChannelsInitializer {
    // To define a new channel, add the channel ID to this StringDef and add a new entry to the
    // CHANNELS map below with the appropriate channel parameters.
    @StringDef({CHANNEL_ID_BROWSER, CHANNEL_ID_SITES})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ChannelId {}
    public static final String CHANNEL_ID_BROWSER = "browser";
    public static final String CHANNEL_ID_SITES = "sites";

    @StringDef({CHANNEL_GROUP_ID_GENERAL})
    @Retention(RetentionPolicy.SOURCE)
    private @interface ChannelGroupId {}
    static final String CHANNEL_GROUP_ID_GENERAL = "general";

    private static final Map<String, Channel> CHANNELS;
    static {
        Map<String, Channel> map = new HashMap<>();
        map.put(CHANNEL_ID_BROWSER,
                new Channel(CHANNEL_ID_BROWSER,
                        org.chromium.chrome.R.string.notification_category_browser,
                        NotificationManager.IMPORTANCE_LOW, CHANNEL_GROUP_ID_GENERAL));
        map.put(CHANNEL_ID_SITES,
                new Channel(CHANNEL_ID_SITES,
                        org.chromium.chrome.R.string.notification_category_sites,
                        NotificationManager.IMPORTANCE_DEFAULT, CHANNEL_GROUP_ID_GENERAL));
        CHANNELS = Collections.unmodifiableMap(map);
    }

    private static final Map<String, ChannelGroup> CHANNEL_GROUPS;
    static {
        Map<String, ChannelGroup> map = new HashMap<>();
        map.put(CHANNEL_GROUP_ID_GENERAL,
                new ChannelGroup(CHANNEL_GROUP_ID_GENERAL,
                        org.chromium.chrome.R.string.notification_category_group_general));
        CHANNEL_GROUPS = Collections.unmodifiableMap(map);
    }

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
        if (!CHANNELS.containsKey(channelId)) {
            throw new IllegalStateException("Could not initialize channel: " + channelId);
        }
        Channel channel = CHANNELS.get(channelId);
        mNotificationManager.createNotificationChannel(channel);
        mNotificationManager.createNotificationChannelGroup(CHANNEL_GROUPS.get(channel.mGroupId));
    }

    /**
     * Helper class containing notification channel properties.
     */
    public static class Channel {
        @ChannelId
        final String mId;
        final int mNameResId;
        final int mImportance;
        @ChannelGroupId
        final String mGroupId;

        Channel(@ChannelId String id, int nameResId, int importance,
                @ChannelGroupId String groupId) {
            this.mId = id;
            this.mNameResId = nameResId;
            this.mImportance = importance;
            this.mGroupId = groupId;
        }
    }

    /**
     * Helper class containing notification channel group properties.
     */
    public static class ChannelGroup {
        @ChannelGroupId
        final String mId;
        final int mNameResId;

        ChannelGroup(@ChannelGroupId String id, int nameResId) {
            this.mId = id;
            this.mNameResId = nameResId;
        }
    }
}
