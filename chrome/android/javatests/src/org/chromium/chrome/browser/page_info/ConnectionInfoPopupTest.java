// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.page_info;

import static org.chromium.chrome.test.util.OmniboxTestUtils.buildSuggestionMap;

import android.os.Build;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;
import android.support.v4.view.ViewCompat;
import android.text.Selection;
import android.util.Pair;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.EnormousTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.FlakyTest;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.ScalableTimeout;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.omnibox.AutocompleteController.OnSuggestionsReceivedListener;
import org.chromium.chrome.browser.omnibox.LocationBarLayout;
import org.chromium.chrome.browser.omnibox.UrlBar;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.chrome.test.util.OmniboxTestUtils;
import org.chromium.chrome.test.util.OmniboxTestUtils.SuggestionsResult;
import org.chromium.chrome.test.util.OmniboxTestUtils.SuggestionsResultBuilder;
import org.chromium.chrome.test.util.OmniboxTestUtils.TestAutocompleteController;
import org.chromium.chrome.test.util.OmniboxTestUtils.TestSuggestionResultsBuilder;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.KeyUtils;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.content.browser.test.util.UiUtils;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.ui.base.DeviceFormFactor;


import org.chromium.net.test.EmbeddedTestServer;

import android.support.test.InstrumentationRegistry;
import org.chromium.content_public.browser.WebContents;

import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Tests for ConnectionInfoPopup.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ConnectionInfoPopupTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    /**
     * Sanity check of ConnectionInfoPopup.
     */
    @Test
    @MediumTest
    @Feature({"ConnectionInfoPopup"})
    @RetryOnFailure
    public void testSimpleUse() throws InterruptedException {

        mActivityTestRule.startMainActivityOnBlankPage();
        final Tab tab = mActivityTestRule.getActivity().getActivityTab();
        // try {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                // ConnectionInfoPopup.show(mActivityTestRule.getActivity(), mActivityTestRule.getActivity().getCurrentContentViewCore().getWebContents());
                ConnectionInfoPopup.show(mActivityTestRule.getActivity(), tab.getWebContents());
            }
        });
    }
}
