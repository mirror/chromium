// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import static org.hamcrest.Matchers.is;
import static org.junit.Assert.assertThat;

import android.app.NotificationManager;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.notifications.MockNotificationManagerProxy;

/**
 * Instrumentation unit tests for ChannelsInitializer.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class ChannelsInitializerTest {
    private ChannelsInitializer mChannelsInitializer;
    private MockNotificationManagerProxy mMockNotificationManager;

    @Before
    public void setUp() throws Exception {
        mMockNotificationManager = new MockNotificationManagerProxy();
        mChannelsInitializer = new ChannelsInitializer(mMockNotificationManager);
    }

    @Test
    @SmallTest
    public void testEnsureInitialized_browserChannel() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelsInitializer.CHANNEL_ID_BROWSER);

        assertThat(mMockNotificationManager.getNotificationChannels().size(), is(1));
        ChannelsInitializer.NotificationChannel channel =
                mMockNotificationManager.getNotificationChannels().get(0);
        assertThat(channel.mId, is(ChannelsInitializer.CHANNEL_ID_BROWSER));
        assertThat(channel.mName, is("Browser"));
        assertThat(channel.mImportance, is(NotificationManager.IMPORTANCE_LOW));
        assertThat(channel.mGroupId, is(ChannelsInitializer.CHANNEL_GROUP_ID_GENERAL));

        assertThat(mMockNotificationManager.getNotificationChannelGroups().size(), is(1));
        ChannelsInitializer.ChannelGroup group =
                mMockNotificationManager.getNotificationChannelGroups().get(0);
        assertThat(group.mId, is(ChannelsInitializer.CHANNEL_GROUP_ID_GENERAL));
        assertThat(group.mName, is("General"));
    }

    @Test
    @SmallTest
    public void testEnsureInitialized_sitesChannel() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelsInitializer.CHANNEL_ID_SITES);

        assertThat(mMockNotificationManager.getNotificationChannels().size(), is(1));

        ChannelsInitializer.NotificationChannel channel =
                mMockNotificationManager.getNotificationChannels().get(0);
        assertThat(channel.mId, is(ChannelsInitializer.CHANNEL_ID_SITES));
        assertThat(channel.mName, is("Sites"));
        assertThat(channel.mImportance, is(NotificationManager.IMPORTANCE_DEFAULT));
        assertThat(channel.mGroupId, is(ChannelsInitializer.CHANNEL_GROUP_ID_GENERAL));

        assertThat(mMockNotificationManager.getNotificationChannelGroups().size(), is(1));
        ChannelsInitializer.ChannelGroup group =
                mMockNotificationManager.getNotificationChannelGroups().get(0);
        assertThat(group.mId, is(ChannelsInitializer.CHANNEL_GROUP_ID_GENERAL));
        assertThat(group.mName, is("General"));
    }
}