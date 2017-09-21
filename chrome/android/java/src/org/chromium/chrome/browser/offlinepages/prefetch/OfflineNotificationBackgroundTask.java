// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages.prefetch;

import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.background_task_scheduler.NativeBackgroundTask;
import org.chromium.chrome.browser.offlinepages.DeviceConditions;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.background_task_scheduler.BackgroundTask.TaskFinishedCallback;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;
import org.chromium.components.background_task_scheduler.TaskParameters;
import org.chromium.net.ConnectionType;

import java.util.Calendar;
import java.util.concurrent.TimeUnit;

/**
 * Handles servicing background offlining requests.
 *
 * Can schedule or cancel tasks, and handles the actual initialization that
 * happens when a task fires.
 */
@JNINamespace("offline_pages::prefetch")
public class OfflineNotificationBackgroundTask extends NativeBackgroundTask {
    public static final long DEFAULT_START_DELAY_MINUTES = 60;
    public static final long OFFLINE_POLL_DELAY_MINUTES = 15;

    // Detection mode used for rescheduling.
    // When online, we reschedule for |DEFAULT_START_DELAY_MINUTES|.
    static final int DETECTION_MODE_ONLINE = 0;

    // When offline, we reschedule for |OFFLINE_POLL_DELAY_MINUTES|.
    static final int DETECTION_MODE_OFFLINE = 1;

    // When we just showed a notification, or are rescheduled due to system upgrades, we wait until
    // the next morning to begin polling again.
    static final int DETECTION_MODE_WAIT_ONE_DAY = 2;

    // Number of notifications that, when ignored consecutively, will cause future notifications to
    // stop.
    static final int IGNORED_NOTIFICATION_MAX = 3;

    // Number of times we will poll offline before deciding that the user is "offline enough" to
    // show a notification.
    static final int OFFLINE_POLLING_ATTEMPTS = 4;

    // If this is set, all calls to get current time will use the timestamp of |sCalendar|.
    private static Calendar sCalendarForTesting;
    private static OfflinePageBridge sOfflinePageBridgeForTesting;

    private TaskFinishedCallback mTaskFinishedCallback;

    public OfflineNotificationBackgroundTask() {}

    private static long delayForDetectionMode(int detectionMode) {
        switch (detectionMode) {
            case DETECTION_MODE_ONLINE:
                return TimeUnit.MINUTES.toMillis(DEFAULT_START_DELAY_MINUTES);
            case DETECTION_MODE_OFFLINE:
                return TimeUnit.MINUTES.toMillis(OFFLINE_POLL_DELAY_MINUTES);
            case DETECTION_MODE_WAIT_ONE_DAY:
                return computeTomorrowMorningMillis() - getCurrentTimeMillis();
            default:
                return -1;
        }
    }

    private static long minimumDelayInMillis() {
        long timeOfLastNotification = PrefetchPrefs.getNotificationLastShownTime();
        if (timeOfLastNotification <= 0) {
            return 0;
        }
        long currentTime = getCurrentTimeMillis();
        if (timeOfLastNotification > currentTime) {
            timeOfLastNotification = currentTime;
        }

        Calendar c = getCalendar();
        c.setTimeInMillis(timeOfLastNotification);
        c.add(Calendar.DAY_OF_MONTH, 1);
        c.set(Calendar.HOUR_OF_DAY, 7);
        c.set(Calendar.MINUTE, 0);
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);
        long minimumNotificationTime = c.getTimeInMillis();
        if (minimumNotificationTime <= currentTime) {
            return 0;
        }

        return minimumNotificationTime - currentTime;
    }

    @CalledByNative
    public static void scheduleTask(int detectionMode) {
        long delayInMillis = delayForDetectionMode(detectionMode);
        long minimumDelay = minimumDelayInMillis();
        if (delayInMillis < minimumDelay) delayInMillis = minimumDelay;
        if (delayInMillis < 0) {
            return;
        }

        TaskInfo taskInfo =
                TaskInfo.createOneOffTask(TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID,
                                OfflineNotificationBackgroundTask.class,
                                // Minimum time to wait
                                delayInMillis,
                                // Maximum time to wait.  After this interval the event will fire
                                // regardless of whether the conditions are right. Since there are
                                // no conditions we shouldn't get to this poitn.
                                delayInMillis * 2)
                        .setIsPersisted(true)
                        .setUpdateCurrent(true)
                        .build();
        BackgroundTaskSchedulerFactory.getScheduler().schedule(
                ContextUtils.getApplicationContext(), taskInfo);
    }

    @Override
    public void reschedule(Context context) {
        if (shouldNotReschedule()) {
            resetPrefs();
            return;
        }

        // This happens only upon upgrade.  So we should be conservative here and determine what we
        // should do after waiting a day.
        scheduleTask(DETECTION_MODE_WAIT_ONE_DAY);
    }

    @Override
    public int onStartTaskBeforeNativeLoaded(
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        assert taskParameters.getTaskId() == TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID;

        if (shouldNotReschedule()) {
            resetPrefs();
            return NativeBackgroundTask.DONE;
        }

        DeviceConditions deviceConditions = DeviceConditions.getCurrentConditions(context);
        if (deviceConditions.getNetConnectionType(context) != ConnectionType.CONNECTION_NONE) {
            PrefetchPrefs.setOfflineCounter(0);
            scheduleTask(DETECTION_MODE_ONLINE);

            // We schedule ourselves and return DONE because we want to reschedule using the normal
            // 1 hour timeout rather than Android's default 30s * 2^n exponential backoff schedule.
            return NativeBackgroundTask.DONE;
        }

        int offlineCounter = PrefetchPrefs.getOfflineCounter();
        offlineCounter++;
        PrefetchPrefs.setOfflineCounter(offlineCounter);
        if (offlineCounter < OFFLINE_POLLING_ATTEMPTS) {
            scheduleTask(DETECTION_MODE_OFFLINE);
            return NativeBackgroundTask.DONE;
        }

        return NativeBackgroundTask.LOAD_NATIVE;
    }

    @Override
    protected void onStartTaskWithNative(
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        mTaskFinishedCallback = callback;
        OfflinePageBridge bridge = getOfflinePageBridge();
        bridge.checkForNewOfflineContent(
                PrefetchPrefs.getNotificationLastShownTime(), this ::doneContentCheck);
    }

    @VisibleForTesting
    void doneContentCheck(String contentHost) {
        resetPrefs();
        mTaskFinishedCallback.taskFinished(false);

        if (contentHost != null) {
            PrefetchPrefs.setNotificationLastShownTime(getCurrentTimeMillis());
            PrefetchedPagesNotifier.getInstance().showNotification(contentHost);
            scheduleTask(DETECTION_MODE_WAIT_ONE_DAY);
        }

        // If there is no fresh content, then we should not have been scheduled in the first place,
        // so don't reschedule anything, and in fact cancel the outstanding task if there is one.
        cancelTask();
    }

    @VisibleForTesting
    protected static long getCurrentTimeMillis() {
        return getCalendar().getTimeInMillis();
    }

    private static void cancelTask() {
        BackgroundTaskSchedulerFactory.getScheduler().cancel(ContextUtils.getApplicationContext(),
                TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID);
    }

    private boolean shouldNotReschedule() {
        return PrefetchPrefs.getIgnoredNotificationCounter() >= IGNORED_NOTIFICATION_MAX
                || !PrefetchPrefs.getHasNewPages();
    }

    private void resetPrefs() {
        PrefetchPrefs.setOfflineCounter(0);
        PrefetchPrefs.setHasNewPages(false);
    }

    /**
     * Computes the number of millis since the Java epoch corresponding to tomorrow at 7am.
     */
    private static long computeTomorrowMorningMillis() {
        Calendar c = getCalendar();
        c.add(Calendar.DAY_OF_MONTH, 1);
        c.set(Calendar.HOUR_OF_DAY, 7);
        c.set(Calendar.MINUTE, 0);
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);
        return c.getTimeInMillis();
    }

    @Override
    protected boolean onStopTaskBeforeNativeLoaded(Context context, TaskParameters taskParameters) {
        return true;
    }

    @Override
    protected boolean onStopTaskWithNative(Context context, TaskParameters taskParameters) {
        assert taskParameters.getTaskId() == TaskIds.OFFLINE_PAGES_PREFETCH_NOTIFICATION_JOB_ID;

        return true;
    }

    // This returns a new instance of Calendar, initialized to the same time as |sCalendar| for
    // testing purposes.
    @VisibleForTesting
    static Calendar getCalendar() {
        Calendar c = Calendar.getInstance();
        if (sCalendarForTesting != null) {
            c.setTimeInMillis(sCalendarForTesting.getTimeInMillis());
        }
        return c;
    }

    @VisibleForTesting
    static OfflinePageBridge getOfflinePageBridge() {
        if (sOfflinePageBridgeForTesting != null) {
            return sOfflinePageBridgeForTesting;
        }
        return OfflinePageBridge.getForProfile(Profile.getLastUsedProfile());
    }

    static void setOfflinePageBridgeForTesting(OfflinePageBridge bridge) {
        sOfflinePageBridgeForTesting = bridge;
    }

    @VisibleForTesting
    static void setCalendarForTesting(Calendar c) {
        sCalendarForTesting = c;
    }
}
