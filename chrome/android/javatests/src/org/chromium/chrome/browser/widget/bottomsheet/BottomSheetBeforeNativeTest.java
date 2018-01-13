// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.ViewMatchers.isEnabled;
import static android.support.test.espresso.matcher.ViewMatchers.withId;

import static org.junit.Assert.assertEquals;

import android.content.Intent;
import android.support.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.browser.ChromeHome;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.ui.test.util.UiRestriction;

/**
 * Tests the behavior of the bottom sheet when used before native is initialized.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // ChromeHome is only enabled on phones
public class BottomSheetBeforeNativeTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityRule = new ChromeTabbedActivityTestRule();

    private ChromeHome.Processor mChromeHomeEnabler = new ChromeHome.Processor();
    private ChromeActivity mActivity;
    private BottomSheet mBottomSheet;

    @Test
    @SmallTest
    public void testOpen_Swipe() {
        startActivity(true);
        setSheetState(BottomSheet.SHEET_STATE_HALF);
        // It is hard to determine when native initialization is finished so this test will not
        // check states of views before native is initialized.
        mActivityRule.waitForActivityNativeInitializationComplete();
        onView(withId(R.id.action_downloads)).check(matches(isEnabled()));
        onView(withId(R.id.action_bookmarks)).check(matches(isEnabled()));
        onView(withId(R.id.action_history)).check(matches(isEnabled()));
    }

    @Test
    @SmallTest
    public void testOpen_OmniboxFocus() throws Exception {
        startActivity(true);
        // Not using espresso actions here so that native initialization is less likely finished.
        // Same for the tests below.
        ThreadUtils.runOnUiThreadBlocking(
                () -> mActivity.findViewById(R.id.url_bar).requestFocus());
        endBottomSheetAnimations();
        checkSheetState(BottomSheet.SHEET_STATE_FULL);
        pressBackButton();
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);
    }

    @Test
    @SmallTest
    public void testOpen_NewTabCreation() {
        // Launching activity with no tabs will trigger a new tab.
        startActivity(false);
        mActivityRule.waitForActivityNativeInitializationComplete();
        endBottomSheetAnimations();
        checkSheetState(BottomSheet.SHEET_STATE_FULL);
        pressBackButton();
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);
    }

    @Test
    @SmallTest
    public void testClose_Swipe() {
        startActivity(true);
        // Open sheet and then to close sheet.
        setSheetState(BottomSheet.SHEET_STATE_HALF);
        checkSheetState(BottomSheet.SHEET_STATE_HALF);
        setSheetState(BottomSheet.SHEET_STATE_PEEK);
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);

        // Open sheet and then to close sheet for a second time.
        setSheetState(BottomSheet.SHEET_STATE_FULL);
        checkSheetState(BottomSheet.SHEET_STATE_FULL);
        mActivityRule.waitForActivityNativeInitializationComplete();
        setSheetState(BottomSheet.SHEET_STATE_PEEK);
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);
    }

    @Test
    @SmallTest
    public void testClose_BackPress() {
        startActivity(true);
        setSheetState(BottomSheet.SHEET_STATE_HALF);
        checkSheetState(BottomSheet.SHEET_STATE_HALF);
        pressBackButton();
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);

        // Open sheet again and press back after native is initialized.
        mActivityRule.waitForActivityNativeInitializationComplete();
        setSheetState(BottomSheet.SHEET_STATE_HALF);
        checkSheetState(BottomSheet.SHEET_STATE_HALF);
        pressBackButton();
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);
    }

    @Test
    @SmallTest
    public void testClose_TabScrim() {
        startActivity(true);
        setSheetState(BottomSheet.SHEET_STATE_HALF);
        checkSheetState(BottomSheet.SHEET_STATE_HALF);
        tabScrim();
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);

        // Try open the sheet to half and tap scrim for a second time.
        setSheetState(BottomSheet.SHEET_STATE_HALF);
        checkSheetState(BottomSheet.SHEET_STATE_HALF);
        tabScrim();
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);

        // Try same action for a third time after native is initialized.
        mActivityRule.waitForActivityNativeInitializationComplete();
        setSheetState(BottomSheet.SHEET_STATE_HALF);
        checkSheetState(BottomSheet.SHEET_STATE_HALF);
        tabScrim();
        checkSheetState(BottomSheet.SHEET_STATE_PEEK);
    }

    /** Simulate back press. */
    private void pressBackButton() {
        ThreadUtils.runOnUiThreadBlocking(mActivity::onBackPressed);
        endBottomSheetAnimations();
    }

    /** Simulate tabbing on the scrim to exit bottom sheet. */
    private void tabScrim() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> { mActivity.findViewById(R.id.fading_focus_target).performClick(); });
        endBottomSheetAnimations();
    }

    /** End bottom sheet animations. */
    private void endBottomSheetAnimations() {
        ThreadUtils.runOnUiThreadBlocking(mBottomSheet::endAnimations);
    }

    /** Set sheet state on UI thread. */
    private void setSheetState(@BottomSheet.SheetState int state) {
        ThreadUtils.runOnUiThreadBlocking(() -> { mBottomSheet.setSheetState(state, false); });
    }

    /** Verify sheet state is in the expected state. */
    private void checkSheetState(@BottomSheet.SheetState int expected) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> assertEquals(expected, mBottomSheet.getSheetState()));
    }

    /**
     * Start ChromeTabbedActivity with either a blank page tab or no tab.
     * @param withTab Whether the activity should start with a tab.
     */
    private void startActivity(boolean withTab) {
        Features.getInstance().enable(ChromeFeatureList.CHROME_HOME);
        mChromeHomeEnabler.setPrefs(true);

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        mActivityRule.prepareUrlIntent(intent, withTab ? "about:blank" : null);
        mActivityRule.startActivityCompletely(intent);

        mActivity = mActivityRule.getActivity();
        CriteriaHelper.pollUiThread(() -> mActivity.getBottomSheet() != null);
        mBottomSheet = mActivity.getBottomSheet();
    }
}
