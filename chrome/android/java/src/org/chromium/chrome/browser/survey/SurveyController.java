// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.app.Activity;
import android.content.Context;

import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.infobar.SurveyInfoBar;

public class SurveyController {
    private static SurveyController sInstance;
    protected String mSurveyId;
    protected String mAdvertisingId;

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
     * Sets the id of the survey to show.
     * @param surveyId The survey's id.
     */
    public void setSurveyId(String surveyId) {
        mSurveyId = surveyId;
    }

    /**
     * Sets the advertising id of the survey to show.
     * @param advertisingId The survey's advertising id.
     */
    public void setAdvertisingId(String advertisingId) {
        mAdvertisingId = advertisingId;
    }

    /**
     * Shows the survey if criteria are met.
     * @return Whether or not to display the survey.
     */
    public boolean shouldShowSurvey() {
        return false;
    }

    /**
     * Show the survey.
     * @param activity The client activity for the survey request.
     * @param context The context to be used for the survey download builder.
     * @param surveyInfoBar The info bar that precedes the survey.
     */
    public void showSurvey(Activity activity, Context context, SurveyInfoBar surveyInfoBar) {
        return;
    }

    /**
     * Register the broadcast receiver to the intent.
     * @param context The context used to create the local broadcast manager.
     */
    public void registerReceiver(Context context) {
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