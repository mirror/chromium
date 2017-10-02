// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.test.filters.MediumTest;
import android.widget.FrameLayout;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ntp.cards.AllDismissedItem.ViewHolder;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.RenderTestRule;
import org.chromium.chrome.test.util.browser.ChromeHome;

import java.io.IOException;


/**
 * Tests for the appearance and behaviour of AllDismissedItem.
 */
//@RunWith(ParameterizedRunner.class)
//@UseRunnerDelegate(ChromeJUnit4RunnerDelegate.class)
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG
})
public class AllDismissedItemTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Rule
    public RenderTestRule mRenderTestRule = new RenderTestRule();

    @SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD")
    @Rule
    public ChromeHome.Processor mChromeHomeStateRule = new ChromeHome.Processor();

    FrameLayout mContentView;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mContentView = new FrameLayout(mActivityTestRule.getActivity());
            mActivityTestRule.getActivity().setContentView(mContentView);
        });
    }

    @Test
    @MediumTest
    @Feature({"Cards", "RenderTest"})
    @ChromeHome(false)
    public void testNewTabPageAppearance() throws IOException {
        // The second parameter (the SectionList) is only called when the item is clicked on, so
        // we can leave it as null for rendering purposes.
        ViewHolder viewHolder = new ViewHolder(mContentView, null);

        renderAtHour(viewHolder, 9, "morning");
        renderAtHour(viewHolder, 14, "afternoon");
        renderAtHour(viewHolder, 20, "evening");
    }

    @Test
    @MediumTest
    @Feature({"Cards", "RenderTest"})
    @ChromeHome
    public void testChromeHomeAppearance() throws IOException {
        renderAtHour(new ViewHolder(mContentView, null), 0, "modern");
    }

    private void renderAtHour(ViewHolder viewHolder, int hour, String renderId) throws IOException {
        // TODO(peconn): Extract common code between this and ArticleSnippetsTest for rendering
        // views in isolation.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            viewHolder.onBindViewHolder(hour);
            mContentView.addView(viewHolder.itemView);
        });
        mRenderTestRule.render(viewHolder.itemView, renderId);
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mContentView.removeView(viewHolder.itemView);
            viewHolder.recycle();
        });
    }
}
