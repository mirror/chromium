// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.app.NotificationManager;
import android.content.Context;
import android.support.annotation.StringDef;

import org.chromium.base.BuildInfo;
import org.chromium.base.Log;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Initializes our notification channels.
 */
public class ChannelsInitializer {
    private static final String TAG = "ChannelsInitializer";

    // To define a new channel, add the channel ID to this StringDef and add a new case to
    // the switch statement in ensureInitialized below with the appropriate channel parameters.
    @StringDef({CHANNEL_ID_BROWSER, CHANNEL_ID_SITES})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ChannelId {}
    public static final String CHANNEL_ID_BROWSER = "browser";
    public static final String CHANNEL_ID_SITES = "sites";

    @StringDef({CHANNEL_GROUP_ID_GENERAL})
    @Retention(RetentionPolicy.SOURCE)
    private @interface ChannelGroupId {}
    private static final String CHANNEL_GROUP_ID_GENERAL = "general";

    public static void ensureInitialized(Context context, @ChannelId String channelId) {
        switch (channelId) {
            case CHANNEL_ID_BROWSER:
                createNotificationChannel(context, CHANNEL_ID_BROWSER,
                        org.chromium.chrome.R.string.notification_category_browser,
                        CHANNEL_GROUP_ID_GENERAL,
                        org.chromium.chrome.R.string.notification_category_group_general,
                        NotificationManager.IMPORTANCE_LOW);
                break;
            case CHANNEL_ID_SITES:
                createNotificationChannel(context, CHANNEL_ID_SITES,
                        org.chromium.chrome.R.string.notification_category_sites,
                        CHANNEL_GROUP_ID_GENERAL,
                        org.chromium.chrome.R.string.notification_category_group_general,
                        NotificationManager.IMPORTANCE_DEFAULT);
                break;
            default:
                throw new RuntimeException("Unhandled channel id: " + channelId);
        }
    }

    /**
     * Creates and registers a notification channel and channel group using the parameters provided.
     * This is a (potentially lengthy) no-op if the channel has already been created.
     */
    private static void createNotificationChannel(Context context, String channelId,
            int channelNameResId, @ChannelGroupId String channelGroupId, int channelGroupNameResId,
            int channelImportance) {
        assert BuildInfo.isAtLeastO();
        NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        /*
        The code in the try-block uses reflection in order to compile as it calls APIs newer than
        our compileSdkVersion of Android. The equivalent code without reflection looks like this:

            notificationManager.createNotificationChannelGroup(
                    new NotificationChannelGroup(channelGroupId, channelGroupName));
            NotificationChannel channel = new NotificationChannel(
                    channelId, channelName, channelImportance);
            channel.setGroup(channelGroupId);
            channel.setShowBadge(false);
            notificationManager.createNotificationChannel(channel);
         */
        // TODO(crbug.com/707804) Stop using reflection once compileSdkVersion is high enough.
        try {
            // Create channel group
            Class<?> channelGroupClass = Class.forName("android.app.NotificationChannelGroup");
            Constructor<?> channelGroupConstructor =
                    channelGroupClass.getDeclaredConstructor(String.class, CharSequence.class);
            Object channelGroup = channelGroupConstructor.newInstance(
                    channelGroupId, context.getString(channelGroupNameResId));

            // Register channel group
            Method createNotificationChannelGroupMethod = notificationManager.getClass().getMethod(
                    "createNotificationChannelGroup", channelGroupClass);
            createNotificationChannelGroupMethod.invoke(notificationManager, channelGroup);

            // Create channel
            Class<?> channelClass = Class.forName("android.app.NotificationChannel");
            Constructor<?> channelConstructor = channelClass.getDeclaredConstructor(
                    String.class, CharSequence.class, int.class);
            Object channel = channelConstructor.newInstance(
                    channelId, context.getString(channelNameResId), channelImportance);

            // Set group on channel
            Method setGroupMethod = channelClass.getMethod("setGroup", String.class);
            setGroupMethod.invoke(channel, channelGroupId);

            // Set channel to not badge on app icon
            Method setShowBadgeMethod = channelClass.getMethod("setShowBadge", boolean.class);
            setShowBadgeMethod.invoke(channel, false);

            // Register channel
            Method createNotificationChannelMethod = notificationManager.getClass().getMethod(
                    "createNotificationChannel", channelClass);
            createNotificationChannelMethod.invoke(notificationManager, channel);

        } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException
                | InstantiationException | InvocationTargetException e) {
            Log.e(TAG, "Error initializing notification channel:", e);
        }
    }
}
