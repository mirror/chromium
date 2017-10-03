// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.app.Activity;
import android.content.Context;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.AppHooks;

/**
 * Class that controls retrieving and displaying surveys. After initializing the controller, clients
 * should call #downloadSurvey() and register a callback to be notified when the survey is
 * available. After downloading the survey, call #showSurveyIfAvailable() to display the survey.
 */
public class SurveyController {
    private static SurveyController sInstance;

    /**
     * @return The SurveyController for use during the lifetime of the browser process.
     */
    public static SurveyController getInstance() {
        if (sInstance == null) {
            sInstance = AppHooks.get().createSurveyController();
        }
        return sInstance;
    }

    /**
     * Register the broadcast receiver to the intent.
     * @param context The context used to create the local broadcast manager.
     * @param siteId The id of the site from where the survey will be downloaded.
     * @param advertisingId Optional advertising id sent with the download request. Pass empty
     *                      string for no advertising id.
     * @param onSurveyAvailableCallback The function to call when the survey is ready.
     *                                  If no survey is available, the callback is ignored.
     */
    public void downloadSurvey(Context context, String siteId, String advertisingId,
            Callback<Void> onSurveyAvailableCallback) {
        return;
    }

    /**
     * Show the survey.
     * @param activity The client activity for the survey request.
     * @param siteId The id of the site from where the survey will be downloaded.
     * @param showAsBottomSheet Whether the survey should be presented as a bottom sheet or not.
     * @param displayLogo Optional resource id of the logo to be displayed on the survey. Pass 0 for
     *                    no logo.
     */
    public void showSurveyIfAvailable(
            Activity activity, String siteId, boolean showAsBottomSheet, int displayLogo) {
        return;
    }

    /**
     * Clears the survey cache containing responses and history.
     * @param context The context used by the survey to clear the cache.
     */
    public void clearCache(Context context) {
        return;
    }
}