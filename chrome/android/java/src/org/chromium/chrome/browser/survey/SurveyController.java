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
 * Class that handles the surveying process from participant filtering to survey displaying.
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
     * Shows the survey if criteria are met.
     * @return Whether or not to display the survey.
     */
    public boolean shouldDownloadSurvey() {
        return false;
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
    public void showSurvey(Activity activity) {
        return;
    }

    /**
     * Register the broadcast receiver to the intent.
     * @param context The context used to create the local broadcast manager.
     * @param webContents The webcontents from where the survey info bar should be displayed.
     * @param surveyId The id of the survey to download.
     * @param advertisingId The advertising id sent with the download request.get
     */
    public void initController(
            Context context, WebContents webContents, String surveyId, String advertisingId) {
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