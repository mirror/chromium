// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;

/**
 * Java counterpart to the C++ NotificationPermissionContext. Provides an accessor for reading the
 * app-level notification permission status that users can disable in System UI.
 *
 * This class should only be used on the UI thread.
 */
public class NotificationPermissionContext {
    private static Boolean sAppLevelNotificationDisabledForTesting;

    /**
     * Returns whether notification permission has been disabled for the entire Android application.
     * This method will return false in cases where this cannot be determined.
     */
    @CalledByNative
    private static boolean areAppLevelNotificationsBlocked() {
        if (sAppLevelNotificationDisabledForTesting != null) {
            sAppLevelNotificationDisabledForTesting = null;
            return true;
        }

        Context context = ContextUtils.getApplicationContext();
        return NotificationSystemStatusUtil.determineAppNotificationStatus(context)
                == NotificationSystemStatusUtil.APP_NOTIFICATIONS_STATUS_DISABLED;
    }

    /**
     * Overrides the app's notification status to be disabled for the next check. Should only be
     * used for testing purposes.
     */
    @CalledByNative
    @VisibleForTesting
    private static void disableAppLevelNotificationStatusForNextCheckForTesting() {
        sAppLevelNotificationDisabledForTesting = true;
    }
}
