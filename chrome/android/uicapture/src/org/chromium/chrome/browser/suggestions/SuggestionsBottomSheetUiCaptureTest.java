// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_PHONE;
import static org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils.registerCategory;

import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.uiautomator.UiDevice;
import android.view.View;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.ScreenShooter;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.BottomSheetContent;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.RecyclerViewTestUtils;
import org.chromium.chrome.test.util.browser.suggestions.DummySuggestionsEventReporter;
import org.chromium.chrome.test.util.browser.suggestions.FakeSuggestionsSource;

/**
 * Tests for the appearance of Article Snippets.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({"enable-features=ChromeHome", ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
@Restriction(RESTRICTION_TYPE_PHONE) // ChromeHome is only enabled on phones
public class SuggestionsBottomSheetUiCaptureTest {
    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();

    private FakeSuggestionsSource mSuggestionsSource;

    private Instrumentation mInstrumentation;

    private ChromeTabbedActivity mActivity;

    private boolean mOldChromeHomeFlagValue;

    private UiDevice mDevice;

    @Before
    public void setup() throws InterruptedException {
        // TODO(dgn,mdjones): Chrome restarts when the ChromeHome feature flag value changes. That
        // crashes the test so we need to manually set the preference to match the flag before
        // staring Chrome.
        ChromePreferenceManager prefManager = ChromePreferenceManager.getInstance();
        mOldChromeHomeFlagValue = prefManager.isChromeHomeEnabled();
        prefManager.setChromeHomeEnabled(true);
        mSuggestionsSource = new FakeSuggestionsSource();
        registerCategory(mSuggestionsSource, /* category = */ 42, /* count = */ 5);
        SuggestionsBottomSheetContent.setSuggestionsSourceForTesting(mSuggestionsSource);
        SuggestionsBottomSheetContent.setEventReporterForTesting(
                new DummySuggestionsEventReporter());
        mActivityTestRule.startMainActivityFromLauncher();
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mActivity = mActivityTestRule.getActivity();
        mDevice = UiDevice.getInstance(mInstrumentation);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mActivity.getBottomSheet().setSheetState(
                        BottomSheet.SHEET_STATE_FULL, /* animate = */ false);
            }
        });
        // The default BottomSheetContent is SuggestionsBottomSheetContent, whose content view is a
        // RecyclerView.
        RecyclerViewTestUtils.waitForStableRecyclerView(
                (SuggestionsRecyclerView) getBottomSheetContent().getContentView().findViewById(
                        R.id.recycler_view));
    }

    @After
    public void tearDown() throws Exception {
        SuggestionsBottomSheetContent.setSuggestionsSourceForTesting(null);
        SuggestionsBottomSheetContent.setEventReporterForTesting(null);
        ChromePreferenceManager.getInstance().setChromeHomeEnabled(mOldChromeHomeFlagValue);
    }

    private BottomSheetContent getBottomSheetContent() {
        return mActivity.getBottomSheet().getCurrentSheetContent();
    }

    @Test
    @MediumTest
    @Feature({"UiCatalogue"})
    public void testBottomSheetPosition() throws Exception {
        mScreenShooter.shoot("Start");
        SuggestionsRecyclerView recyclerView =
                (SuggestionsRecyclerView) getBottomSheetContent().getContentView().findViewById(
                        R.id.recycler_view);

        BottomSheet bottomSheet = mActivity.getBottomSheet();
        int dragHeight = bottomSheet.getTop() - bottomSheet.getBottom();
        int[] location = new int[2];
        bottomSheet.getLocationOnScreen(location);
        int middleX = location[0] + bottomSheet.getWidth() / 2;
        int bottomY = location[1] + bottomSheet.getHeight() / 2;
        int topY = bottomY - dragHeight;
        mDevice.swipe(middleX, bottomY, middleX, bottomY + dragHeight, 1);
        mScreenShooter.shoot("drag_up");
        mDevice.swipe(middleX, topY, middleX, bottomY, 1);
        mScreenShooter.shoot("drag_down");
    }

    @Test
    @MediumTest
    @Feature({"UiCatalogue"})
    public void testContextMenu() throws Exception {
        mScreenShooter.shoot("Start");
        SuggestionsRecyclerView recyclerView =
                (SuggestionsRecyclerView) getBottomSheetContent().getContentView().findViewById(
                        R.id.recycler_view);
        View firstCardView = RecyclerViewTestUtils.waitForView(recyclerView, 2).itemView;
        int[] location = new int[2];
        firstCardView.getLocationOnScreen(location);
        int firstCardX = location[0] + firstCardView.getWidth() / 2;
        int firstCardY = location[1] + firstCardView.getHeight() / 2;
        mDevice.swipe(firstCardX, firstCardY, firstCardX, firstCardY, 100);
        mScreenShooter.shoot("contextMenu");
    }
}
