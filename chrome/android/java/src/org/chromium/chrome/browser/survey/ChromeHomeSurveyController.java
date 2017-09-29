// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.Callback;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.StrictModeContext;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.infobar.SurveyInfoBar;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.components.variations.VariationsAssociatedData;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;

/**
 * Class that controls if and when to show surveys related to Chrome Home.
 */
public class ChromeHomeSurveyController {
    private static final String TRIAL_NAME = "ChromeHome";
    private static final String PARAM_NAME = "survey_id";
    private static final long ONE_WEEK_IN_MILLIS = 604800000L;

    private TabModelSelector mTabModelSelector;

    public ChromeHomeSurveyController(TabModelSelector tabModelSelector) {
        mTabModelSelector = tabModelSelector;
    }

    public void kickoffSurveyProcess(Tab tab, Context context) {
        SurveyController surveyController = SurveyController.getInstance();
        if (tab != null && context != null && userQualifiesForSurvey()) {
            CommandLine instance = CommandLine.getInstance();
            String surveyId;
            if (instance.hasSwitch(PARAM_NAME)) {
                surveyId = instance.getSwitchValue(PARAM_NAME);
            } else {
                surveyId = VariationsAssociatedData.getVariationParamValue(TRIAL_NAME, PARAM_NAME);
            }
            if (surveyId == null) return;
            surveyController.initialize(context, surveyId, true, R.drawable.chrome_sync_logo,
                    new SurveyAvailableCallback());
            surveyController.downloadSurvey(context, surveyId, "");
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

    /**
     * Callback class to check survey showing criteria and show the survey if they are met.
     */
    @SuppressWarnings("rawtypes")
    public class SurveyAvailableCallback implements Callback {
        private String mSurveyId;
        private boolean mShowAsBottomSheet;
        private int mDisplayLogo;

        @Override
        public void onResult(Object result) {
            Tab tab = mTabModelSelector.getCurrentTab();
            if (tab != null && tab.getWebContents() != null) {
                WebContents webContents = tab.getWebContents();
                if (webContents.isLoading()) {
                    webContents.addObserver(new WebContentsObserver() {
                        @Override
                        public void didStopLoading(String url) {
                            SurveyInfoBar.showSurveyInfoBar(
                                    webContents, mSurveyId, mShowAsBottomSheet, mDisplayLogo);
                        }
                    });
                } else {
                    SurveyInfoBar.showSurveyInfoBar(
                            webContents, mSurveyId, mShowAsBottomSheet, mDisplayLogo);
                }
            }
        }

        public void initSurveyParams(String surveyId, boolean showAsBottomSheet, int displayLogo) {
            mSurveyId = surveyId;
            mShowAsBottomSheet = showAsBottomSheet;
            mDisplayLogo = displayLogo;
        }
    }
}