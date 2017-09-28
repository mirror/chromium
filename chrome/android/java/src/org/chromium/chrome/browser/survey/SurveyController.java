// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.app.Activity;
import android.content.Context;

import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.infobar.SurveyInfoBar;
import org.chromium.content_public.browser.WebContents;

/**
 * Class that controls retrieving and displaying surveys.
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
     * Downloads the survey fitting the passed in parameters.
     * Shows {@link SurveyInfoBar} on success.
     * @param context The context to be used for the survey download builder.
     */
    public void downloadSurvey(Context context) {
        return;
    }

    /**
     * Show the survey.
     * @param activity The client activity for the survey request.
     */
    public void showSurveyIfAvailable(Activity activity) {
        return;
    }

    /**
     * Register the broadcast receiver to the intent.
     * @param context The context used to create the local broadcast manager.
     * @param webContents The {@link WebContents} used to display the {@link SurveyInfoBar}.
     * @param surveyId The id of the survey to download.
     * @param advertisingId The advertising id sent with the download request.
     * @param showAsBottomSheet Whether the survey should be presented as a bottom sheet or not.
     * @param displayLogo The resource id of the logo to be displayed on the survey.
     */
    public void initController(Context context, WebContents webContents, String surveyId,
            String advertisingId, boolean showAsBottomSheet, int displayLogo) {
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