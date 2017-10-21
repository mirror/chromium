// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.content.SharedPreferences;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for ChromeHomeSurveyController.java.
 * Complemented by javatests/src/org/chromium/chrome/browser/survey/ChromeHomeSurveyController.java.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ChromeHomeSurveyControllerTest {
    ChromeHomeSurveyController mController;

    SharedPreferences mSharedPreferences;

    @Before
    public void before() {
        ContextUtils.initApplicationContextForTests(RuntimeEnvironment.application);
        mController = ChromeHomeSurveyController.createChromeHomeSurveyController();
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
        mSharedPreferences.edit().clear().apply();
    }

    @After
    public void after() {
        mSharedPreferences.edit().clear().apply();
    }

    @Test
    public void testInfoBarDisplayedBefore() {
        Assert.assertFalse(mController.hasInfoBarBeenDisplayed());
        Assert.assertFalse(
                mSharedPreferences.contains(ChromeHomeSurveyController.SURVEY_INFO_BAR_DISPLAYED));
        mSharedPreferences.edit()
                .putBoolean(ChromeHomeSurveyController.SURVEY_INFO_BAR_DISPLAYED, true)
                .apply();
        Assert.assertTrue(mController.hasInfoBarBeenDisplayed());
    }

    @Test
    public void testChromeHomeEnabledForOneWeek() {
        Assert.assertFalse(mController.wasChromeHomeEnabledForMinimumOneWeek());
        Assert.assertFalse(mSharedPreferences.contains(
                ChromePreferenceManager.CHROME_HOME_SHARED_PREFERENCES_KEY));
        mSharedPreferences.edit()
                .putLong(ChromePreferenceManager.CHROME_HOME_SHARED_PREFERENCES_KEY,
                        System.currentTimeMillis() - ChromeHomeSurveyController.ONE_WEEK_IN_MILLIS)
                .apply();
        Assert.assertTrue(mController.wasChromeHomeEnabledForMinimumOneWeek());
    }

    @Test
    public void testChromeHomeEnabledForLessThanOneWeek() {
        Assert.assertFalse(mController.wasChromeHomeEnabledForMinimumOneWeek());
        Assert.assertFalse(mSharedPreferences.contains(
                ChromePreferenceManager.CHROME_HOME_SHARED_PREFERENCES_KEY));
        mSharedPreferences.edit()
                .putLong(ChromePreferenceManager.CHROME_HOME_SHARED_PREFERENCES_KEY,
                        System.currentTimeMillis()
                                - ChromeHomeSurveyController.ONE_WEEK_IN_MILLIS / 2)
                .apply();
        Assert.assertFalse(mController.wasChromeHomeEnabledForMinimumOneWeek());
    }
}