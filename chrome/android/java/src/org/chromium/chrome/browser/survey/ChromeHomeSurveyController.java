// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.StrictModeContext;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.infobar.InfoBarContainer;
import org.chromium.chrome.browser.infobar.InfoBarContainerLayout.Item;
import org.chromium.chrome.browser.infobar.InfoBarIdentifier;
import org.chromium.chrome.browser.infobar.SurveyInfoBar;
import org.chromium.chrome.browser.infobar.SurveyInfoBarDelegate;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.privacy.PrivacyPreferencesManager;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;
import org.chromium.chrome.browser.util.AccessibilityUtil;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.components.variations.VariationsAssociatedData;
import org.chromium.content_public.browser.WebContents;

/**
 * Class that controls if and when to show surveys related to the Chrome Home experiment.
 */
public class ChromeHomeSurveyController implements InfoBarContainer.InfoBarAnimationListener {
    public static final String SURVEY_INFO_BAR_DISPLAYED_KEY =
            "chrome_home_survey_info_bar_displayed";
    public static final String PARAM_NAME = "survey_override_site_id";

    private static final String TRIAL_NAME = "ChromeHome";
    private static final long REQUIRED_VISIBILITY_DURATION_MS = 5000;

    static final long ONE_WEEK_IN_MILLIS = 604800000L;
    static final String SURVEY_INFOBAR_DISMISSED_KEY = "chrome_home_survey_info_bar_dismissed";

    private TabModelSelector mTabModelSelector;
    private Handler mLoggingHandler;
    private Tab mSurveyInfoBarTab;
    private TabModelSelectorObserver mTabModelObserver;

    @VisibleForTesting
    ChromeHomeSurveyController() {
        // Empty constructor.
    }

    /**
     * Checks if the conditions to show the survey are met and starts the process if they are.
     * @param context The current Android {@link Context}.
     * @param tabModelSelector The tab model selector to access the tab on which the survey will be
     *                         shown.
     */
    public static void initialize(Context context, TabModelSelector tabModelSelector) {
        new ChromeHomeSurveyController().startDownload(context, tabModelSelector);
    }

    /**
     * Downloads the survey if the user qualifies.
     * @param context The current Android {@link Context}.
     * @param tabModelSelector The tab model selector to access the tab on which the survey will be
     *                         shown.
     */
    private void startDownload(Context context, TabModelSelector tabModelSelector) {
        if (!doesUserQualifyForSurvey()) return;

        mLoggingHandler = new Handler();
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

        Runnable onSuccessRunnable = new Runnable() {
            @Override
            public void run() {
                onSurveyAvailable(siteId);
            }
        };
        surveyController.downloadSurvey(context, siteId, onSuccessRunnable);
    }

    /** @return Whether the user qualifies for the survey. */
    private boolean doesUserQualifyForSurvey() {
        if (CommandLine.getInstance().hasSwitch(ChromeSwitches.CHROME_HOME_FORCE_ENABLE_SURVEY)) {
            return true;
        }
        if (!isUMAEnabled()) return false;
        if (AccessibilityUtil.isAccessibilityEnabled()) return false;
        if (hasInfoBarBeenDisplayed()) return false;
        if (!FeatureUtilities.isChromeHomeEnabled()) return true;
        return wasChromeHomeEnabledForMinimumOneWeek();
    }

    /**
     * Called when the survey has finished downloading to show the survey info bar.
     * @param siteId The site id of the survey to display.
     */
    @VisibleForTesting
    void onSurveyAvailable(String siteId) {
        mSurveyInfoBarTab = mTabModelSelector.getCurrentTab();
        if (isValidTabForSurvey(mSurveyInfoBarTab) && mSurveyInfoBarTab.isUserInteractable()) {
            WebContents webContents = mSurveyInfoBarTab.getWebContents();
            if (!webContents.isLoading()) {
                showSurveyInfoBar(mSurveyInfoBarTab, siteId);
                return;
            }
            mSurveyInfoBarTab.addObserver(getTabObserver(mSurveyInfoBarTab, siteId));
        }
        mTabModelObserver = getTabModelSelectorObserver(siteId);
        mTabModelSelector.addObserver(mTabModelObserver);
    }

    /**
     * Shows the survey info bar.
     * @param tab The tab to attach the survey info bar.
     * @param siteId The site id of the survey to display.
     */
    @VisibleForTesting
    void showSurveyInfoBar(Tab tab, String siteId) {
        mSurveyInfoBarTab = tab;
        tab.getInfoBarContainer().addAnimationListener(this);
        SurveyInfoBar.showSurveyInfoBar(tab.getWebContents(), siteId, true,
                R.drawable.chrome_sync_logo, getSurveyInfoBarDelegate());
    }

    /** @return The observer that adds a tab observer to the current tab. */
    private TabModelSelectorObserver getTabModelSelectorObserver(String siteId) {
        return new EmptyTabModelSelectorObserver() {
            @Override
            public void onChange() {
                if (!isValidTabForSurvey(mTabModelSelector.getCurrentTab())) return;
                Tab tab = mTabModelSelector.getCurrentTab();
                tab.addObserver(getTabObserver(tab, siteId));
            }
        };
    }

    /** @return The observer that handles cases where the user switches tabs before an infobar is
     * shown. */
    @VisibleForTesting
    TabObserver getTabObserver(Tab tab, String siteId) {
        return new EmptyTabObserver() {
            @Override
            public void onInteractabilityChanged(boolean isInteractable) {
                if (!isInteractable) {
                    return;
                }
                WebContents webContents = tab.getWebContents();
                if (!webContents.isLoading()) {
                    showSurveyInfoBar(tab, siteId);
                    mTabModelSelector.removeObserver(mTabModelObserver);
                    tab.removeObserver(this);
                    return;
                }
            }

            @Override
            public void onPageLoadFinished(Tab tab) {
                if (!tab.isUserInteractable()) {
                    return;
                }
                showSurveyInfoBar(tab, siteId);
                mTabModelSelector.removeObserver(mTabModelObserver);
                tab.removeObserver(this);
            }

            @Override
            public void onHidden(Tab tab) {
                tab.removeObserver(this);
            }
        };
    }

    /** @return Whether the user has consented to reporting usage metrics and crash dumps. */
    private boolean isUMAEnabled() {
        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            return PrivacyPreferencesManager.getInstance()
                    .isUsageAndCrashReportingPermittedByUser();
        }
    }

    /** @return If the survey info bar was logged as seen before. */
    @VisibleForTesting
    boolean hasInfoBarBeenDisplayed() {
        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            return sharedPreferences.getBoolean(SURVEY_INFO_BAR_DISPLAYED_KEY, false);
        }
    }

    /** @return If it has been over a week since ChromeHome was enabled. */
    @VisibleForTesting
    boolean wasChromeHomeEnabledForMinimumOneWeek() {
        try (StrictModeContext unused = StrictModeContext.allowDiskReads()) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            long earliestLoggedDate = sharedPreferences.getLong(
                    ChromePreferenceManager.CHROME_HOME_SHARED_PREFERENCES_KEY, Long.MAX_VALUE);
            if (System.currentTimeMillis() - earliestLoggedDate >= ONE_WEEK_IN_MILLIS) return true;
        }
        return false;
    }

    /**
     * Checks if the tab is valid for a survey (i.e. not null, no null webcontents & not incognito).
     * @param tab The tab to be checked.
     * @return Whether or not the tab is valid.
     */
    @VisibleForTesting
    boolean isValidTabForSurvey(Tab tab) {
        return tab != null && tab.getWebContents() != null && !tab.isIncognito();
    }

    /** @return A new {@link ChromeHomeSurveyController} for testing. */
    @VisibleForTesting
    public static ChromeHomeSurveyController createChromeHomeSurveyControllerForTests() {
        return new ChromeHomeSurveyController();
    }

    @Override
    public void notifyAnimationFinished(int animationType) {}

    @Override
    public void notifyAllAnimationsFinished(Item frontInfoBar) {
        if (frontInfoBar == null) return;

        mLoggingHandler.removeCallbacksAndMessages(null);

        // If the survey info bar is in front, start the countdown to log that it was displayed.
        if (frontInfoBar.getInfoBarIdentifier() != InfoBarIdentifier.SURVEY_INFOBAR_ANDROID) {
            return;
        }
        mLoggingHandler.postDelayed(
                () -> recordInfoBarDisplayed(), REQUIRED_VISIBILITY_DURATION_MS);
    }

    /**
     * @return The survey info bar delegate containing actions specific to the Chrome Home survey.
     */
    private SurveyInfoBarDelegate getSurveyInfoBarDelegate() {
        return new SurveyInfoBarDelegate() {

            @Override
            public void onSurveyInfoBarClosed() {
                recordInfoBarDisplayed();
            }

            @Override
            public void onSurveyTriggered() {
                RecordUserAction.record("Android.ChromeHome.AcceptedSurvey");
            }

            @Override
            public String getSurveyPromptString() {
                return ContextUtils.getApplicationContext().getString(
                        R.string.chrome_home_survey_prompt);
            }
        };
    }

    /** Logs in {@link SharedPreferences} that the info bar was displayed. */
    private void recordInfoBarDisplayed() {
        mSurveyInfoBarTab.getInfoBarContainer().removeAnimationListener(this);
        mLoggingHandler.removeCallbacksAndMessages(null);
        if (!mSurveyInfoBarTab.isUserInteractable()) return;
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        sharedPreferences.edit().putBoolean(SURVEY_INFOBAR_DISMISSED_KEY, true).apply();
    }

    @VisibleForTesting
    void setTabModelSelector(TabModelSelector tabModelSelector) {
        mTabModelSelector = tabModelSelector;
    }
}
