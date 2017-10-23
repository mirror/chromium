// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.survey;

import android.os.Looper;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Java tests for ChromeHomeSurveyController.java.
 * Complemented by junit/src/org/chromium/chrome/browser/survey/ChromeHomeSurveyController.java.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
public class ChromeHomeSurveyControllerTest {
    ChromeHomeSurveyController mController =
            ChromeHomeSurveyController.createChromeHomeSurveyController();

    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Test
    @SmallTest
    public void testValidTab() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
        Tab validTab = mActivityTestRule.getActivity().getTabModelSelector().getCurrentTab();
        Assert.assertTrue(mController.isValidTabForSurvey(validTab));
    }

    @Test
    @SmallTest
    public void testNullTab() {
        Assert.assertFalse(mController.isValidTabForSurvey(null));
    }

    @Test
    @SmallTest
    public void testIncognitoTab() {
        Looper.prepare();
        Tab tab = new Tab(0, true, null);
        Assert.assertTrue(tab != null);
        Assert.assertFalse(mController.isValidTabForSurvey(tab));
    }

    @Test
    @SmallTest
    public void testTabWithNoWebContents() {
        Looper.prepare();
        Tab tab = new Tab(0, false, null);
        Assert.assertTrue(tab != null);
        Assert.assertFalse(tab.isIncognito());
        Assert.assertTrue(tab.getWebContents() == null);
        Assert.assertFalse(mController.isValidTabForSurvey(tab));
    }
}
