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

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
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

    private static final String DEBUG_PAGE = "chrome://ukm";

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    public String getElementContent(Tab normalTab, String elementId) throws Exception {
        mActivityTestRule.loadUrlInTab(
                DEBUG_PAGE, PageTransition.TYPED | PageTransition.FROM_ADDRESS_BAR, normalTab);
        return JavaScriptUtils.executeJavaScriptAndWaitForResult(
                normalTab.getContentViewCore().getWebContents(),
                "document.getElementById('" + elementId + "').textContent");
    }

    public String getUkmState(Tab normalTab) throws Exception {
        return getElementContent(normalTab, "state");
    }

    public String getUkmClientId(Tab normalTab) throws Exception {
        return getElementContent(normalTab, "clientid");
    }

    /**
     * Closes the current tab.
     * @param incognito Whether to close an incognito or non-incognito tab.
     */
    protected void closeCurrentTab(final boolean incognito) throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mActivityTestRule.getActivity().getTabModelSelector().selectModel(incognito);
            }
        });
        ChromeTabUtils.closeCurrentTab(
                InstrumentationRegistry.getInstrumentation(), mActivityTestRule.getActivity());
    }

    @Test
    @SmallTest
    @RetryOnFailure
    public void testRegularPlusIncognitoCheck() throws Exception {
        Tab normalTab = mActivityTestRule.getActivity().getActivityTab();

        Assert.assertEquals("UKM State:", "\"True\"", getUkmState(normalTab));

        String clientId = getUkmClientId(normalTab);
        Assert.assertFalse("Non empty client id:", clientId.isEmpty());

        mActivityTestRule.newIncognitoTabFromMenu();

        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        // Opening another regular tab mustn't enable UKM.
        ChromeTabUtils.newTabFromMenu(
                InstrumentationRegistry.getInstrumentation(), mActivityTestRule.getActivity());
        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        // Opening and closing another Incognito tab mustn't enable UKM.
        mActivityTestRule.newIncognitoTabFromMenu();

        closeCurrentTab(true /* incognito */);
        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        closeCurrentTab(false /* non-incognito */);
        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        closeCurrentTab(true /* incognito */);
        Assert.assertEquals("UKM State:", "\"True\"", getUkmState(normalTab));

        // Client ID should not have been reset.
        Assert.assertEquals("Client id:", clientId, getUkmClientId(normalTab));
    }

    @Test
    @SmallTest
    @RetryOnFailure
    public void testIncognitoPlusRegularCheck() throws Exception {
        // Start by closing all tabs.
        ChromeTabUtils.closeAllTabs(
                InstrumentationRegistry.getInstrumentation(), mActivityTestRule.getActivity());

        mActivityTestRule.newIncognitoTabFromMenu();

        ChromeTabUtils.newTabFromMenu(
                InstrumentationRegistry.getInstrumentation(), mActivityTestRule.getActivity());
        Tab normalTab = mActivityTestRule.getActivity().getActivityTab();

        Assert.assertEquals("UKM State:", "\"False\"", getUkmState(normalTab));

        closeCurrentTab(true /* incognito */);
        Assert.assertEquals("UKM State:", "\"True\"", getUkmState(normalTab));
    }
}
