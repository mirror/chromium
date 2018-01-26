// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.metrics;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.content.browser.test.util.JavaScriptUtils;
import org.chromium.ui.base.PageTransition;

/**
 * Tests for UKM monitoring of incognito activity.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.
Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "force-enable-metrics-reporting"})
public class UkmIncognitoTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    TabModel mIncognito, mNonIncognito;

    private static final String DEBUG_PAGE = "chrome://ukm";

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();

        mIncognito = mActivityTestRule.getActivity().getTabModelSelector().getModel(
                true /* incognito */);
        mNonIncognito = mActivityTestRule.getActivity().getTabModelSelector().getModel(
                false /* !incognito */);
    }

    public String getUkmState(Tab normalTab) throws Exception {
        mActivityTestRule.loadUrlInTab(
                DEBUG_PAGE, PageTransition.TYPED | PageTransition.FROM_ADDRESS_BAR, normalTab);
        return JavaScriptUtils.executeJavaScriptAndWaitForResult(
                normalTab.getContentViewCore().getWebContents(),
                "document.getElementById('state').textContent");
    }

    @Test
    @SmallTest
    @RetryOnFailure
    public void testRegularPlusIncognitoCheck() throws Exception {
        Tab normalTab = mActivityTestRule.getActivity().getActivityTab();

        Assert.assertEquals("UKM State:", "\"True\"", getUkmState(normalTab));

        // TODO(bmcquade): extract original_client_id from DEBUG_PAGE.

        mActivityTestRule.newIncognitoTabFromMenu();

        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        // Opening another regular tab mustn't enable UKM.
        ChromeTabUtils.newTabFromMenu(
                InstrumentationRegistry.getInstrumentation(), mActivityTestRule.getActivity());
        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        // Opening and closing another Incognito tab mustn't enable UKM.
        mActivityTestRule.newIncognitoTabFromMenu();
        TabModelUtils.closeCurrentTab(mIncognito);
        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        TabModelUtils.closeCurrentTab(mNonIncognito);
        normalTab = TabModelUtils.getCurrentTab(mNonIncognito);
        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        mIncognito.closeAllTabs();
        Assert.assertEquals("UKM State:", "\"True\"", getUkmState(normalTab));

        // Client ID should not have been reset.
        // TODO(bmcquade): extract client id from DEBUG_PAGE.
    }

    @Test
    @SmallTest
    @RetryOnFailure
    public void testIncognitoPlusRegularCheck() throws Exception {
        // Close all non-incognito tabs.
        mNonIncognito.closeAllTabs();

        mActivityTestRule.newIncognitoTabFromMenu();

        ChromeTabUtils.newTabFromMenu(
                InstrumentationRegistry.getInstrumentation(), mActivityTestRule.getActivity());
        Tab normalTab = TabModelUtils.getCurrentTab(mNonIncognito);

        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        TabModelUtils.closeCurrentTab(mNonIncognito);

        Assert.assertEquals("UKM State:", "\"True\"", getUkmState(normalTab));
    }
}
