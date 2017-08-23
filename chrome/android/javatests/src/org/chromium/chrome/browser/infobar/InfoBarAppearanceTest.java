// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.support.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.ScreenShooter;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.InfoBarTestAnimationListener;

/**
 * Tests for the appearance of InfoBars.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
@ScreenShooter.Directory("InfoBars")
public class InfoBarAppearanceTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();

    private InfoBarTestAnimationListener mListener;
    private Tab mTab;

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();

        mListener = new InfoBarTestAnimationListener();

        mTab = mActivityTestRule.getActivity().getActivityTab();
        mTab.getInfoBarContainer().addAnimationListener(mListener);
    }

    @Test
    @MediumTest
    @Feature({"InfoBars", "Catalogue"})
    public void testNotifyInfoBar() throws Exception {
        TestNotifyInfobarDelegate infoBarDelegate = new TestNotifyInfobarDelegate();
        NotifyInfoBar infobar = new NotifyInfoBar(infoBarDelegate);

        ThreadUtils.runOnUiThreadBlocking(() -> mTab.getInfoBarContainer().addInfoBar(infobar));
        mListener.addInfoBarAnimationFinished("InfoBar was not added.");
        mScreenShooter.shoot("compact");

        ThreadUtils.runOnUiThreadBlocking(infobar::onLinkClicked);
        mListener.swapInfoBarAnimationFinished("InfoBar did not expand.");
        mScreenShooter.shoot("expanded");

        ThreadUtils.runOnUiThreadBlocking(infobar::onLinkClicked);
        infoBarDelegate.linkTappedHelper.waitForCallback("link was not tapped.", 0);
    }

    private static class TestNotifyInfobarDelegate implements NotifyInfoBarDelegate {
        public final CallbackHelper linkTappedHelper = new CallbackHelper();

        @Override
        public String getDescription() {
            return "This is the long description for a notify infobar. FYI stuff happened.";
        }

        @Override
        public String getShortDescription() {
            return "Stuff happened.";
        }

        @Override
        public String getFeaturedLinkText() {
            return "Click to say it works";
        }

        @Override
        public int getIconResourceId() {
            return R.drawable.star_green;
        }

        @Override
        public void onLinkTapped() {
            linkTappedHelper.notifyCalled();
        }
    }
}
