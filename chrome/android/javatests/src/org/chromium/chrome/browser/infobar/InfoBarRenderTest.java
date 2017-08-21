// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import static junit.framework.Assert.assertNotNull;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.ScreenShooter;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.RenderTestRule;
import org.chromium.chrome.test.util.browser.TabTitleObserver;

import java.util.List;

import javax.annotation.Nullable;

/**
 * Tests for the appearance of InfoBars.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
@ScreenShooter.Directory("InfoBars")
public class InfoBarRenderTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Rule
    public RenderTestRule mRenderTestRule =
            new RenderTestRule("chrome/test/data/android/render_tests");

    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();

    private static final String URL1 =
            UrlUtils.encodeHtmlDataUri("<html><head></head><body>foo</body></html>");
    private static final String URL2 = UrlUtils.encodeHtmlDataUri(
            "<html><head><title>done</title></head><body>foo</body></html>");

    @Test
    @MediumTest
    @Feature({"InfoBars", "RenderTest", "Catalogue"})
    @DisabledTest(message = "need to disable infobar animations - crbug.com/755582")
    public void testFramebustBlockInfoBar() throws Exception {
        mActivityTestRule.startMainActivityWithURL(URL1);
        Tab tab = mActivityTestRule.getActivity().getActivityTab();
        ThreadUtils.runOnUiThreadBlocking(
                () -> tab.getTabWebContentsDelegateAndroid().onDidBlockFramebust(URL2));
        Thread.sleep(1500); // TODO(dgn): remove

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        InfoBar infobar = findByIdentifier(InfoBarIdentifier.FRAMEBUST_BLOCK_INFOBAR_ANDROID);
        assertNotNull(infobar);

        mRenderTestRule.render(infobar.getView(), "compact");
        mScreenShooter.shoot("compact");

        ThreadUtils.runOnUiThreadBlocking(infobar::onLinkClicked);
        Thread.sleep(1500); // TODO(dgn): remove

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        mRenderTestRule.render(infobar.getView(), "expanded");
        mScreenShooter.shoot("expanded");

        ThreadUtils.runOnUiThreadBlocking(infobar::onLinkClicked);
        new TabTitleObserver(tab, "done").waitForTitleUpdate(1);
    }

    @Nullable
    private InfoBar findByIdentifier(@InfoBarIdentifier int id) {
        List<InfoBar> infobars = mActivityTestRule.getActivity()
                                         .getActivityTab()
                                         .getInfoBarContainer()
                                         .getInfoBarsForTesting();
        for (InfoBar infobar : infobars) {
            if (infobar.getInfoBarIdentifier() == id) return infobar;
        }
        return null;
    }
}
