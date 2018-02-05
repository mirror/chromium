// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.support.test.filters.SmallTest;
import android.view.View;
import android.widget.TextView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.search_engines.TemplateUrlService.LoadListener;
import org.chromium.chrome.browser.toolbar.ToolbarModelImpl;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Unit tests for {@link LocationBarLayout}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class LocationBarLayoutTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Rule
    public TestRule mProcessor = new Features.InstrumentationProcessor();

    private static final String SEARCH_TERMS = "machine learning";
    private static final String GOOGLE_SRP_URL = "https://www.google.com/search?q=machine+learning";
    private static final String GOOGLE_VERBOSE_TEXT = "Google";

    private static final String BING_SRP_URL = "https://www.bing.com/search?q=machine+learning";
    private static final String BING_VERBOSE_TEXT = "Bing";

    private TestToolbarModel mTestToolbarModel;

    private class TestToolbarModel extends ToolbarModelImpl {
        public TestToolbarModel(BottomSheet bottomSheet) {
            super(bottomSheet);
            initializeWithNative();
        }

        private String mCurrentUrl;
        private Integer mSecurityLevel;

        void setCurrentUrl(String url) {
            mCurrentUrl = url;
        }

        void setSecurityLevel(@ConnectionSecurityLevel int securityLevel) {
            mSecurityLevel = securityLevel;
        }

        @Override
        public String getCurrentUrl() {
            if (mCurrentUrl == null) return super.getCurrentUrl();
            android.util.Log.w("thildebr", "returning " + mCurrentUrl);
            return mCurrentUrl;
        }

        @Override
        @ConnectionSecurityLevel
        public int getSecurityLevel() {
            if (mSecurityLevel == null) return super.getSecurityLevel();
            return mSecurityLevel;
        }
    }

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
        mTestToolbarModel = new TestToolbarModel(mActivityTestRule.getActivity().getBottomSheet());
        mTestToolbarModel.setTab(mActivityTestRule.getActivity().getActivityTab(), false);
        getLocationBar().setToolbarDataProvider(mTestToolbarModel);
    }

    private void setUrlToPageUrl(LocationBarLayout locationBar) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                locationBar.setUrlToPageUrl();
                // Trigger onUrlFocusChange to update the verbose status visibility.
                locationBar.onUrlFocusChange(false);
            }
        });
    }

    // Partially lifted from TemplateUrlServiceTest.
    private void setSearchEngine(String keyword) {
        final AtomicBoolean observerNotified = new AtomicBoolean(false);
        final LoadListener listener = new LoadListener() {
            @Override
            public void onTemplateUrlServiceLoaded() {
                observerNotified.set(true);
            }
        };
        ThreadUtils.runOnUiThreadBlocking(() -> {
            TemplateUrlService service = TemplateUrlService.getInstance();
            service.registerLoadListener(listener);
            service.load();
            service.setSearchEngine(keyword);
        });

        CriteriaHelper.pollInstrumentationThread(
                new Criteria("Observer wasn't notified of TemplateUrlService load.") {
                    @Override
                    public boolean isSatisfied() {
                        return observerNotified.get();
                    }
                });
    }

    private UrlBar getUrlBar() {
        return (UrlBar) mActivityTestRule.getActivity().findViewById(R.id.url_bar);
    }

    private TextView getVerboseStatusTextView() {
        return (TextView) mActivityTestRule.getActivity().findViewById(
                R.id.location_bar_verbose_status);
    }

    private LocationBarLayout getLocationBar() {
        return (LocationBarLayout) mActivityTestRule.getActivity().findViewById(R.id.location_bar);
    }

    private TintedImageButton getSecurityButton() {
        return (TintedImageButton) mActivityTestRule.getActivity().findViewById(
                R.id.security_button);
    }

    @Test
    @SmallTest
    @EnableFeatures(ChromeFeatureList.QUERY_IN_OMNIBOX)
    @Feature({"QueryInOmnibox"})
    public void testDontShowVerboseStatusIfNotSrpOrOfflinePage() {
        final TextView verboseStatusTextView = getVerboseStatusTextView();
        Assert.assertFalse(verboseStatusTextView.getVisibility() == View.VISIBLE);
    }

    @Test
    @SmallTest
    @EnableFeatures(ChromeFeatureList.QUERY_IN_OMNIBOX)
    @Feature({"QueryInOmnibox"})
    public void testShowVerboseStatusIfSrpForGoogle() {
        final LocationBarLayout locationBar = getLocationBar();

        mTestToolbarModel.setCurrentUrl(GOOGLE_SRP_URL);
        setUrlToPageUrl(locationBar);

        final TextView verboseStatusTextView = getVerboseStatusTextView();
        Assert.assertTrue(verboseStatusTextView.getVisibility() == View.VISIBLE);
        Assert.assertEquals(GOOGLE_VERBOSE_TEXT, verboseStatusTextView.getText());
    }

    @Test
    @SmallTest
    @EnableFeatures(ChromeFeatureList.QUERY_IN_OMNIBOX)
    @Feature({"QueryInOmnibox"})
    public void testIsOnlyShowingSearchTermsIfSrp() {
        final UrlBar urlBar = getUrlBar();
        final LocationBarLayout locationBar = getLocationBar();

        mTestToolbarModel.setCurrentUrl(GOOGLE_SRP_URL);
        setUrlToPageUrl(locationBar);

        final TextView verboseStatusTextView = getVerboseStatusTextView();
        Assert.assertEquals(SEARCH_TERMS, urlBar.getText().toString());
    }

    @Test
    @SmallTest
    @EnableFeatures(ChromeFeatureList.QUERY_IN_OMNIBOX)
    @Feature({"QueryInOmnibox"})
    public void testShowVerboseStatusIfSrpForBing() {
        final LocationBarLayout locationBar = getLocationBar();

        setSearchEngine("bing.com");
        mTestToolbarModel.setCurrentUrl(BING_SRP_URL);
        setUrlToPageUrl(locationBar);

        final TextView verboseStatusTextView = getVerboseStatusTextView();
        Assert.assertTrue(verboseStatusTextView.getVisibility() == View.VISIBLE);
        Assert.assertEquals(BING_VERBOSE_TEXT, verboseStatusTextView.getText());
    }

    @Test
    @SmallTest
    @EnableFeatures(ChromeFeatureList.QUERY_IN_OMNIBOX)
    @Feature({"QueryInOmnibox"})
    public void testShowingInfoIconOnMixedContent() {
        final LocationBarLayout locationBar = getLocationBar();

        mTestToolbarModel.setCurrentUrl(GOOGLE_SRP_URL);
        setUrlToPageUrl(locationBar);

        mTestToolbarModel.setSecurityLevel(ConnectionSecurityLevel.HTTP_SHOW_WARNING);

        TintedImageButton securityButton = getSecurityButton();
        Assert.assertTrue(securityButton.getVisibility() == View.VISIBLE);
    }

    @Test
    @SmallTest
    @DisableFeatures(ChromeFeatureList.QUERY_IN_OMNIBOX)
    @Feature({"QueryInOmnibox"})
    public void testDontShowDseVerboseStatusIfQueryInOmniboxFlagDisabled() {
        final LocationBarLayout locationBar = getLocationBar();

        mTestToolbarModel.setCurrentUrl(GOOGLE_SRP_URL);
        setUrlToPageUrl(locationBar);

        final TextView verboseStatusTextView = getVerboseStatusTextView();
        Assert.assertFalse(verboseStatusTextView.getVisibility() == View.VISIBLE);
    }
}