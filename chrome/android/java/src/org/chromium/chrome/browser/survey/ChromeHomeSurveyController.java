// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.content.Context;
import android.content.SharedPreferences;
import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.StrictModeContext;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.infobar.SurveyInfoBar;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.components.variations.VariationsAssociatedData;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;

/**
 * Class that controls if and when to show surveys related to Chrome Home.
 */
public class ChromeHomeSurveyController {
    private static final String SURVEY_INFO_BAR_DISPLAYED = "survey_info_bar_displayed";
    private static final String SURVEY_FORCE_ENABLE_SWITCH = "force-enable-survey";
    private static final String TRIAL_NAME = "ChromeHome";
    private static final String PARAM_NAME = "site_id";
    private static final long ONE_WEEK_IN_MILLIS = 604800000L;

    private TabModelSelector mTabModelSelector;
    private String mSiteId;
    private boolean mShowAsBottomSheet;
    private int mDisplayLogo;

    /**
     * Checks if the conditions to show the survey are met and starts the process if they are.
     * @param context The current Android {@link Context}.
     * @param tabModelSelector The tab model selector to access the tab on which the survey will be
     *                         shown.
     */
    public void initialize(Context context, TabModelSelector tabModelSelector) {
        if (!doesUserQualifyForSurvey()) return;

        mTabModelSelector = tabModelSelector;

        SurveyController surveyController = SurveyController.getInstance();
        CommandLine commandLine = CommandLine.getInstance();
        String siteId;
        if (commandLine.hasSwitch(PARAM_NAME)) {
            siteId = commandLine.getSwitchValue(PARAM_NAME);
        } else {
            siteId = VariationsAssociatedData.getVariationParamValue(TRIAL_NAME, PARAM_NAME);
        }

        if (TextUtils.isEmpty(siteId)) return;

        mSiteId = siteId;
        mShowAsBottomSheet = true;
        mDisplayLogo = R.drawable.chrome_sync_logo;
        Callback<Void> surveyAvailableCallback = new Callback<Void>() {
            @Override
            public void onResult(Void result) {
                onSurveyAvailable();
            }
        };
        surveyController.downloadSurvey(context, siteId, "", surveyAvailableCallback);
    }

    private boolean doesUserQualifyForSurvey() {
        if (!FeatureUtilities.isChromeHomeEnabled()) return true;
        if (CommandLine.getInstance().hasSwitch(SURVEY_FORCE_ENABLE_SWITCH)) return true;
        if (hasInfoBarBeenDisplayed()) return true;

        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            long earliestLoggedDate = sharedPreferences.getLong(
                    ChromePreferenceManager.CHROME_HOME_SHARED_PREFERENCES_KEY, Long.MAX_VALUE);
            if (System.currentTimeMillis() - earliestLoggedDate > ONE_WEEK_IN_MILLIS) return true;
        }
        return false;
    }

    private void onSurveyAvailable() {
        Tab tab = mTabModelSelector.getCurrentTab();
        if (tab == null || tab.getWebContents() == null) return;

        WebContents webContents = tab.getWebContents();
        if (webContents.isLoading()) {
            webContents.addObserver(new WebContentsObserver() {
                @Override
                public void didFinishLoad(long frameId, String validatedUrl, boolean isMainFrame) {
                    if (isMainFrame) {
                        SurveyInfoBar.showSurveyInfoBar(
                                webContents, mSiteId, mShowAsBottomSheet, mDisplayLogo);
                        logInfoBarDisplayed();
                        webContents.removeObserver(this);
                    }
                }
            });
        } else {
            SurveyInfoBar.showSurveyInfoBar(webContents, mSiteId, mShowAsBottomSheet, mDisplayLogo);
            logInfoBarDisplayed();
        }
    }

    private void logInfoBarDisplayed() {
        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            sharedPreferences.edit().putBoolean(SURVEY_INFO_BAR_DISPLAYED, true).apply();
        }
    }

    private boolean hasInfoBarBeenDisplayed() {
        boolean infoBarBeenDisplayed;
        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            infoBarBeenDisplayed = sharedPreferences.getBoolean(SURVEY_INFO_BAR_DISPLAYED, false);
        }
        return infoBarBeenDisplayed;
    }
}