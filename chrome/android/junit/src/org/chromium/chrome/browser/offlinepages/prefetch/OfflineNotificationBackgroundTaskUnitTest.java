// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages.prefetch;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.Context;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.multidex.ShadowMultiDex;

import org.chromium.base.BaseChromiumApplication;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.offlinepages.DeviceConditions;
import org.chromium.chrome.browser.offlinepages.ShadowDeviceConditions;
import org.chromium.components.background_task_scheduler.BackgroundTask.TaskFinishedCallback;
import org.chromium.components.background_task_scheduler.BackgroundTaskScheduler;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;
import org.chromium.components.background_task_scheduler.TaskParameters;
import org.chromium.net.ConnectionType;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.concurrent.TimeUnit;

/** Unit tests for {@link OfflineNotificationBackgroundTask}. */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE, application = BaseChromiumApplication.class,
        shadows = {ShadowMultiDex.class, ShadowDeviceConditions.class})
public class OfflineNotificationBackgroundTaskUnitTest {
    /**
     * Fake of BackgroundTaskScheduler system service.
     */
    public static class FakeBackgroundTaskScheduler implements BackgroundTaskScheduler {
        private HashMap<Integer, TaskInfo> mTaskInfos = new HashMap<>();

        @Override
        public boolean schedule(Context context, TaskInfo taskInfo) {
            mTaskInfos.put(taskInfo.getTaskId(), taskInfo);
            return true;
        }

        @Override
        public void cancel(Context context, int taskId) {
            mTaskInfos.remove(taskId);
        }

        @Override
        public void checkForOSUpgrade(Context context) {}

        @Override
        public void reschedule(Context context) {}

        public TaskInfo getTaskInfo(int taskId) {
            return mTaskInfos.get(taskId);
        }

        public void clear() {
            mTaskInfos = new HashMap<>();
        }
    }

    @Spy
    private OfflineNotificationBackgroundTask mOfflineNotificationBackgroundTask =
            new OfflineNotificationBackgroundTask();
    @Mock
    private ChromeBrowserInitializer mChromeBrowserInitializer;
    @Captor
    ArgumentCaptor<BrowserParts> mBrowserParts;
    private FakeBackgroundTaskScheduler mFakeTaskScheduler;
    private Calendar mCalendar;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        doNothing().when(mChromeBrowserInitializer).handlePreNativeStartup(any(BrowserParts.class));
        try {
            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock invocation) {
                    mBrowserParts.getValue().finishNativeInitialization();
                    return null;
                }
            })
                    .when(mChromeBrowserInitializer)
                    .handlePostNativeStartup(eq(true), mBrowserParts.capture());
        } catch (ProcessInitException ex) {
            fail("Unexpected exception while initializing mock of ChromeBrowserInitializer.");
        }

        ChromeBrowserInitializer.setForTesting(mChromeBrowserInitializer);

        mFakeTaskScheduler = new FakeBackgroundTaskScheduler();
        BackgroundTaskSchedulerFactory.setSchedulerForTesting(mFakeTaskScheduler);

        mCalendar = Calendar.getInstance();
        // Emulate it being January 1, 2017 00:00:00 at the start of each test.
        mCalendar.clear();
        mCalendar.set(2017, 1, 1, 0, 0, 0);

        OfflineNotificationBackgroundTask.setCalendarForTesting(mCalendar);
    }

    public void assertTaskScheduledForOnlineDelay(TaskInfo task) {
        long delayInMillis = TimeUnit.MINUTES.toMillis(
                OfflineNotificationBackgroundTask.DEFAULT_START_DELAY_MINUTES);
        assertNotNull(task);
        assertEquals(delayInMillis, task.getOneOffInfo().getWindowStartTimeMs());
        assertEquals(true, task.isPersisted());
    }

    @Test
    public void scheduleTaskAsIfOnline() {
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_ONLINE);
        TaskInfo scheduledTask =
                mFakeTaskScheduler.getTaskInfo(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);
        assertNotNull(scheduledTask);
        assertTaskScheduledForOnlineDelay(scheduledTask);
    }

    @Test
    public void scheduleTaskAsIfOffline() {
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_OFFLINE);
        TaskInfo scheduledTask =
                mFakeTaskScheduler.getTaskInfo(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);
        assertNotNull(scheduledTask);
        assertEquals(TimeUnit.MINUTES.toMillis(
                             OfflineNotificationBackgroundTask.OFFLINE_POLL_DELAY_MINUTES),
                scheduledTask.getOneOffInfo().getWindowStartTimeMs());
        assertEquals(true, scheduledTask.isPersisted());
    }

    @Test
    public void scheduleTaskAsIfWaitingForADay() {
        Calendar expectedTimeSchedule = Calendar.getInstance();
        expectedTimeSchedule.clear();
        expectedTimeSchedule.set(2017, 1, 2, 7, 0, 0);

        long expectedDelayMillis =
                expectedTimeSchedule.getTimeInMillis() - mCalendar.getTimeInMillis();

        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_WAIT_ONE_DAY);
        TaskInfo scheduledTask =
                mFakeTaskScheduler.getTaskInfo(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);
        assertNotNull(scheduledTask);
        assertEquals(expectedDelayMillis, scheduledTask.getOneOffInfo().getWindowStartTimeMs());
        assertEquals(true, scheduledTask.isPersisted());
    }

    @Test
    public void startTaskWhileOnline() {
        final ArrayList<Boolean> reschedules = new ArrayList<>();
        TaskParameters params = mock(TaskParameters.class);
        when(params.getTaskId()).thenReturn(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);

        // Setup battery conditions with no power connected.
        DeviceConditions deviceConditions = new DeviceConditions(true /* POWER_CONNECTED */,
                75 /* BATTERY_LEVEL */, ConnectionType.CONNECTION_WIFI, false /* POWER_SAVE */);
        ShadowDeviceConditions.setCurrentConditions(deviceConditions, false /* metered */);

        mOfflineNotificationBackgroundTask.onStartTask(null, params, new TaskFinishedCallback() {
            @Override
            public void taskFinished(boolean needsReschedule) {
                reschedules.add(needsReschedule);
            }
        });

        assertEquals(1, reschedules.size());
        assertEquals(false, reschedules.get(0));

        TaskInfo scheduledTask =
                mFakeTaskScheduler.getTaskInfo(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);
        assertNotNull(scheduledTask);
        assertTaskScheduledForOnlineDelay(scheduledTask);
    }
}
