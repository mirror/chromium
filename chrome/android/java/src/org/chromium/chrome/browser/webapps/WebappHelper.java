// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import static org.chromium.webapk.lib.common.WebApkConstants.WEBAPK_PACKAGE_PREFIX;

import android.text.TextUtils;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.browser.ChromeActivity;

/**
 * A helper class that contains shared logic between different forms of WebappActivity that are not
 * used by all modes. This helper is owned by a {@link WebappActivity} instance.
 */
public class WebappHelper {
    /** Records whether we're currently showing a disclosure notification. */
    private boolean mNotificationShowing;

    /**
     * Register the owning {@link WebappActivity} through its id to the {@link WebappRegistry}.
     * @param id The id of the owning activity.
     * @param callback The callback to call after the registration is complete.
     */
    void register(String id, WebappRegistry.FetchWebappDataStorageCallback callback) {
        // Register the WebAPK. The WebAPK was registered when it was created, but may also become
        // unregistered after a user clears Chrome's data.
        WebappRegistry.getInstance().register(id, callback);
    }

    /**
     * If we're showing a WebApk that's not with an expected package, it must be an
     * "Unbound WebApk" (crbug.com/714735) so show a notification that it's running in Chrome.
     */
    void maybeShowDisclosure(WebappActivity activity, WebappDataStorage storage) {
        String webAPKPackageName = activity.getWebApkPackageName();
        if ((TextUtils.isEmpty(webAPKPackageName)
                    || webAPKPackageName.startsWith(WEBAPK_PACKAGE_PREFIX))
                && !storage.hasDismissedDisclosure() && !mNotificationShowing
                && !WebappActionsNotificationManager.isEnabled()) {
            int activityState = ApplicationStatus.getStateForActivity(activity);
            if (activityState == ActivityState.STARTED || activityState == ActivityState.RESUMED
                    || activityState == ActivityState.PAUSED) {
                mNotificationShowing = true;
                WebApkDisclosureNotificationManager.showDisclosure(activity.mWebappInfo);
            }
        }
    }

    /**
     * Called when the owning {@link WebappActivity} gets {@link ChromeActivity#onStopWithNative()}
     * called.
     * @param info {@link WebappInfo} for the owning activity.
     */
    void onStopWithNative(WebappInfo info) {
        if (mNotificationShowing) {
            WebApkDisclosureNotificationManager.dismissNotification(info);
            mNotificationShowing = false;
        }
    }
}
