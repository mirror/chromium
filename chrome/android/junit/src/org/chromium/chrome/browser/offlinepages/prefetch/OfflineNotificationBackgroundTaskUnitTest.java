// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages.prefetch;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
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
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.multidex.ShadowMultiDex;

import org.chromium.base.BaseChromiumApplication;
import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.offlinepages.DeviceConditions;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.ShadowDeviceConditions;
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
    @Mock
    OfflinePageBridge mOfflinePageBridge;
    @Mock
    PrefetchedPagesNotifier mPrefetchedPagesNotifier;

    private FakeBackgroundTaskScheduler mFakeTaskScheduler;
    private Calendar mCalendar;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        // Set up the context.
        ContextUtils.initApplicationContextForTests(RuntimeEnvironment.application);
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

        doAnswer((invocation) -> {
            Object callback = invocation.getArguments()[1];
            ((Callback<String>) callback).onResult("www.example.com");
            return null;
        })
                .when(mOfflinePageBridge)
                .checkForNewOfflineContent(anyLong(), any(Callback.class));
        OfflineNotificationBackgroundTask.setOfflinePageBridgeForTesting(mOfflinePageBridge);
        PrefetchedPagesNotifier.setInstanceForTest(mPrefetchedPagesNotifier);

        ChromeBrowserInitializer.setForTesting(mChromeBrowserInitializer);

        mFakeTaskScheduler = new FakeBackgroundTaskScheduler();
        BackgroundTaskSchedulerFactory.setSchedulerForTesting(mFakeTaskScheduler);

        mCalendar = Calendar.getInstance();
        // Emulate it being January 1, 2017 00:00:00 at the start of each test.
        mCalendar.clear();
        mCalendar.set(2017, 1, 1, 0, 0, 0);

        OfflineNotificationBackgroundTask.setCalendarForTesting(mCalendar);
        clearPrefs();
    }

    private void clearPrefs() {
        ContextUtils.getAppSharedPreferences().edit().clear().apply();
    }

    /**
     * Runs mOfflineNotificationBackgroundTask with the given params.
     * Asserts that reschedule was called exactly once and returns the reschedule value.
     */
    private void runTask() {
        TaskParameters params = mock(TaskParameters.class);
        when(params.getTaskId()).thenReturn(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);

        final ArrayList<Boolean> reschedules = new ArrayList<>();
        mOfflineNotificationBackgroundTask.onStartTask(RuntimeEnvironment.application, params,
                (reschedule) -> reschedules.add(reschedule));

        assertEquals(0, reschedules.size());
    }

    private void runTaskAndExpectTaskDone() {
        TaskParameters params = mock(TaskParameters.class);
        when(params.getTaskId()).thenReturn(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);

        final ArrayList<Boolean> reschedules = new ArrayList<>();
        mOfflineNotificationBackgroundTask.onStartTask(RuntimeEnvironment.application, params,
                (reschedule) -> reschedules.add(reschedule));

        assertEquals(false, reschedules.get(0));
    }

    private void setupDeviceOnlineStatus(boolean online) {
        DeviceConditions deviceConditions =
                new DeviceConditions(false /* POWER_CONNECTED */, 75 /* BATTERY_LEVEL */,
                        online ? ConnectionType.CONNECTION_WIFI : ConnectionType.CONNECTION_NONE,
                        false /* POWER_SAVE */);
        ShadowDeviceConditions.setCurrentConditions(deviceConditions, false /* metered */);
    }

    public void assertTaskScheduledForOfflineDelay() {
        assertTaskScheduledForDelay(OfflineNotificationBackgroundTask.OFFLINE_POLL_DELAY_MINUTES);
    }

    public void assertTaskScheduledForOnlineDelay() {
        assertTaskScheduledForDelay(OfflineNotificationBackgroundTask.DEFAULT_START_DELAY_MINUTES);
    }

    public void assertTaskScheduledForTomorrowMorning() {
        // 31 hours = 24 hours + 7 (7am on January 2, 2017).
        assertTaskScheduledForDelay(31 * 60);
    }

    public void assertTaskScheduledForDelay(long delay) {
        TaskInfo task =
                mFakeTaskScheduler.getTaskInfo(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);
        assertNotNull(task);
        long delayInMillis = TimeUnit.MINUTES.toMillis(delay);
        assertNotNull(task);
        assertEquals(delayInMillis, task.getOneOffInfo().getWindowStartTimeMs());
        assertEquals(true, task.isPersisted());
    }

    public void assertNoTaskScheduled() {
        TaskInfo task =
                mFakeTaskScheduler.getTaskInfo(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);
        assertNull(task);
    }

    private void assertNativeDidNotStart() {
        verify(mChromeBrowserInitializer, never()).handlePreNativeStartup(any(BrowserParts.class));
    }

    private void assertNativeStarted() {
        verify(mChromeBrowserInitializer, atLeastOnce())
                .handlePreNativeStartup(any(BrowserParts.class));
    }

    private void assertNotificationNotShown() {
        verify(mPrefetchedPagesNotifier, never()).showNotification("www.example.com");
    }

    private void assertNotificationShown() {
        verify(mPrefetchedPagesNotifier, atLeastOnce()).showNotification("www.example.com");
        assertEquals(false, PrefetchPrefs.getHasNewPages());
    }

    @Test
    public void scheduleTaskAsIfOnline() {
        PrefetchPrefs.setHasNewPages(true);
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_ONLINE);
        assertTaskScheduledForOnlineDelay();
    }

    @Test
    public void scheduleTaskAsIfOffline() {
        PrefetchPrefs.setHasNewPages(true);
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_OFFLINE);
        assertTaskScheduledForOfflineDelay();
    }

    @Test
    public void startTaskWithNoNewPages() {
        PrefetchPrefs.setHasNewPages(false);
        setupDeviceOnlineStatus(false);

        runTask();

        assertNoTaskScheduled();
        assertNativeDidNotStart();
    }

    @Test
    public void startTaskWhileOnline() {
        PrefetchPrefs.setHasNewPages(true);
        setupDeviceOnlineStatus(true);

        runTask();

        assertTaskScheduledForOnlineDelay();
        assertNativeDidNotStart();
    }

    @Test
    public void startTaskWhileOffline() {
        PrefetchPrefs.setHasNewPages(true);
        setupDeviceOnlineStatus(false);

        runTask();

        assertTaskScheduledForOfflineDelay();
        assertNativeDidNotStart();
    }

    @Test
    public void offlineCounterSchedulesNotification() {
        PrefetchPrefs.setHasNewPages(true);
        setupDeviceOnlineStatus(false);

        // Run the task almost enough times.
        for (int i = 0; i < OfflineNotificationBackgroundTask.OFFLINE_POLLING_ATTEMPTS - 1; i++) {
            runTask();
            assertNativeDidNotStart();
        }
        runTaskAndExpectTaskDone();
        assertNativeStarted();
        assertNotificationShown();
    }

    @Test
    public void onlineResetsOfflineCounter() {
        PrefetchPrefs.setHasNewPages(true);
        setupDeviceOnlineStatus(false);

        // Run the task almost enough times.
        for (int i = 0; i < OfflineNotificationBackgroundTask.OFFLINE_POLLING_ATTEMPTS - 1; i++) {
            runTask();
            assertNativeDidNotStart();
        }
        setupDeviceOnlineStatus(true);
        // Run once while online.
        runTask();
        assertNativeDidNotStart();

        setupDeviceOnlineStatus(false);
        // Run the task almost enough times.
        for (int i = 0; i < OfflineNotificationBackgroundTask.OFFLINE_POLLING_ATTEMPTS - 1; i++) {
            runTask();
            assertNativeDidNotStart();
        }

        // Then run it the final time and expect a notification.
        runTaskAndExpectTaskDone();
        assertNativeStarted();
        assertNotificationShown();
    }

    @Test
    public void showingNotificationCausesADelayUntilTomorrow() {
        // Setup task as though online
        PrefetchPrefs.setHasNewPages(true);
        setupDeviceOnlineStatus(true);
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_ONLINE);
        assertTaskScheduledForOnlineDelay();

        // Then simulate offline-ness and cause notification.
        setupDeviceOnlineStatus(false);
        for (int i = 0; i < OfflineNotificationBackgroundTask.OFFLINE_POLLING_ATTEMPTS - 1; i++) {
            runTask();
        }
        runTaskAndExpectTaskDone();

        // Then restart again, see if the next task is scheduled.
        setupDeviceOnlineStatus(true);
        PrefetchPrefs.setHasNewPages(true);
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_ONLINE);
        assertTaskScheduledForTomorrowMorning();

        // Fast forward past 7am tomorrow morning.
        mCalendar.add(Calendar.DATE, 1);
        mCalendar.set(Calendar.HOUR_OF_DAY, 8);

        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_ONLINE);
        assertTaskScheduledForOnlineDelay();
    }

    @Test
    public void ignoredNotificationPreventsSchedulingTask() {
        PrefetchPrefs.setHasNewPages(true);
        setupDeviceOnlineStatus(true);
        PrefetchPrefs.setIgnoredNotificationCounter(
                OfflineNotificationBackgroundTask.IGNORED_NOTIFICATION_MAX);
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_ONLINE);
        assertNoTaskScheduled();
        setupDeviceOnlineStatus(false);
        OfflineNotificationBackgroundTask.scheduleTask(
                OfflineNotificationBackgroundTask.DETECTION_MODE_OFFLINE);
        assertNoTaskScheduled();
    }

    @Test
    public void ignoredNotificationPreventsNotificationShow() {
        // Set up the prefs so that a notification would be shown, if not for the ignored
        // notification counter.
        PrefetchPrefs.setHasNewPages(true);
        PrefetchPrefs.setOfflineCounter(OfflineNotificationBackgroundTask.OFFLINE_POLLING_ATTEMPTS);
        setupDeviceOnlineStatus(false);

        // Set up the ignored notification counter.
        PrefetchPrefs.setIgnoredNotificationCounter(
                OfflineNotificationBackgroundTask.IGNORED_NOTIFICATION_MAX);
        runTask();
        assertNativeDidNotStart();
        assertNoTaskScheduled();
    }
}
