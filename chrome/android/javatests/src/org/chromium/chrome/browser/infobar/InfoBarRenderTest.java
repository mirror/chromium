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
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.DisableAnimationsRule;
import org.chromium.chrome.test.util.RenderTestRule;

import java.io.IOException;
import java.util.List;

import javax.annotation.Nullable;

/**
 * Tests for the appearance of Article Snippets.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class InfoBarRenderTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);
    @Rule
    public RenderTestRule mRenderTestRule =
            new RenderTestRule("chrome/test/data/android/render_tests");

    @SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD") // Rules must be public.
    @Rule
    public DisableAnimationsRule mDisableChromeAnimationsRule = new DisableAnimationsRule();

    private static final String URL =
            UrlUtils.encodeHtmlDataUri("<html><head></head><body>foo</body></html>");

    @Test
    @MediumTest
    @Feature({"InfoBars", "RenderTest"})
    public void testFramebustBlockInfoBar() throws IOException, InterruptedException {
        mActivityTestRule.startMainActivityWithURL(URL);
        ThreadUtils.runOnUiThreadBlocking(() -> {
            Tab tab = mActivityTestRule.getActivity().getActivityTab();
            FramebustBlockInfoBar.showFramebustBlockInfoBar(
                    tab, "http://www.isyourcomputerinfected.biz/");
        });
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        InfoBar infobar = findByIdentifier(InfoBarIdentifier.FRAMEBUST_BLOCK_INFOBAR_ANDROID);
        assertNotNull(infobar);

        mRenderTestRule.render(infobar.getView(), "compact");

        infobar.onLinkClicked();
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        mRenderTestRule.render(infobar.getView(), "expanded");
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
