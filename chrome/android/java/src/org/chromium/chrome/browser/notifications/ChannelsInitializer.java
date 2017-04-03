// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.app.NotificationManager;
import android.content.Context;

import org.chromium.base.Log;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Initializes our notification channels.
 */
public class ChannelsInitializer {
    private static final String TAG = "ChannelsInitializer";
    private final Context mContext;
    private final NotificationManager mNotificationManager;

    public ChannelsInitializer(Context context) {
        mContext = context;
        mNotificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    public void ensureInitialized(String channelId) {
        switch (channelId) {
            case NotificationConstants.CHANNEL_ID_BROWSER:
                createNotificationChannel(NotificationConstants.CHANNEL_ID_BROWSER,
                        org.chromium.chrome.R.string.notification_category_browser,
                        NotificationConstants.CHANNEL_GROUP_ID_GENERAL,
                        org.chromium.chrome.R.string.notification_category_group_general,
                        NotificationManager.IMPORTANCE_LOW);
                break;
            case NotificationConstants.CHANNEL_ID_SITES:
                createNotificationChannel(NotificationConstants.CHANNEL_ID_SITES,
                        org.chromium.chrome.R.string.notification_category_sites,
                        NotificationConstants.CHANNEL_GROUP_ID_GENERAL,
                        org.chromium.chrome.R.string.notification_category_group_general,
                        NotificationManager.IMPORTANCE_DEFAULT);
                break;
            default:
                throw new IllegalArgumentException("Unknown notification channel id: " + channelId);
        }
    }

    private void createNotificationChannel(String channelId, int channelNameResId,
            String channelGroupId, int channelGroupNameResId, int channelImportance) {
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
        // TODO(crbug.com/707804) Stop using reflection once compileSdkVersion is bumped to 26.
        try {
            // Create channel group
            Class<?> channelGroupClass = Class.forName("android.app.NotificationChannelGroup");
            Constructor<?> channelGroupConstructor =
                    channelGroupClass.getDeclaredConstructor(String.class, CharSequence.class);
            Object channelGroup = channelGroupConstructor.newInstance(
                    channelGroupId, mContext.getString(channelGroupNameResId));

            // Register channel group
            Method createNotificationChannelGroupMethod = mNotificationManager.getClass().getMethod(
                    "createNotificationChannelGroup", channelGroupClass);
            createNotificationChannelGroupMethod.invoke(mNotificationManager, channelGroup);

            // Create channel
            Class<?> channelClass = Class.forName("android.app.NotificationChannel");
            Constructor<?> channelConstructor = channelClass.getDeclaredConstructor(
                    String.class, CharSequence.class, int.class);
            Object channel = channelConstructor.newInstance(
                    channelId, mContext.getString(channelNameResId), channelImportance);

            // Set group on channel
            Method setGroupMethod = channelClass.getMethod("setGroup", String.class);
            setGroupMethod.invoke(channel, channelGroupId);

            // Set channel to not badge on app icon
            Method setShowBadgeMethod = channelClass.getMethod("setShowBadge", boolean.class);
            setShowBadgeMethod.invoke(channel, false);

            // Register channel
            Method createNotificationChannelMethod = mNotificationManager.getClass().getMethod(
                    "createNotificationChannel", channelClass);
            createNotificationChannelMethod.invoke(mNotificationManager, channel);

        } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException
                | InstantiationException | InvocationTargetException e) {
            Log.e(TAG, "Error initializing notification channel:", e);
        }
    }
}
