// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.StrictModeContext;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.variations.VariationsAssociatedData;

/**
 * Class that controls if and when to show surveys related to Chrome Home.
 */
public class ChromeHomeSurveyController {
    private static final String TRIAL_NAME = "ChromeHome";
    private static final String PARAM_NAME = "survey_id";
    private static final long ONE_WEEK_IN_MILLIS = 604800000L;

    public void initSurveyController(Tab tab, Context context) {
        android.util.Log.d("INJAE", "ChromeHomeSurveyController#initSurveyController");
        SurveyController surveyController = SurveyController.getInstance();
        if (tab != null && context != null && userQualifiesForSurvey()) {
            CommandLine instance = CommandLine.getInstance();
            String surveyId;
            if (instance.hasSwitch(PARAM_NAME)) {
                surveyId = instance.getSwitchValue(PARAM_NAME);
            }
            surveyId = VariationsAssociatedData.getVariationParamValue(TRIAL_NAME, PARAM_NAME);
            if (surveyId == null) return;
            surveyId = "re5jgmht4qljymubtbyzwc3tpm";
            surveyController.initController(
                    context, tab.getWebContents(), surveyId, "", true, R.drawable.chrome_sync_logo);
            SurveyController.getInstance().downloadSurvey(context);
        }
    }

    private boolean userQualifiesForSurvey() {
        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            long earliestLoggedDate = sharedPreferences.getLong(
                    ChromePreferenceManager.CHROME_HOME_SHARED_PREFERENCES_KEY, Long.MAX_VALUE);
            if (System.currentTimeMillis() - earliestLoggedDate > ONE_WEEK_IN_MILLIS) return true;
        }
        return false;
    }
}