// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.download.ui.ThumbnailProvider.ThumbnailRequest;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.concurrent.Callable;

/**
 * Instrumentation test for {@link ThumbnailProviderImpl}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
})
public class ThumbnailProviderImplTest {
    @Rule
    public SuggestionsDependenciesRule mSuggestionsDeps = new SuggestionsDependenciesRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private ThumbnailProviderImpl mThumbnailProvider;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
        mThumbnailProvider =
                (ThumbnailProviderImpl) mSuggestionsDeps.getFactory().createThumbnailProvider();
        clearThumbnailCache();
    }

    @After
    public void tearDown() {
        clearThumbnailCache();
    }

    @Test
    @MediumTest
    @Feature({"Suggestions"})
    public void testBothThumbnailSidesSmallerThanRequiredSize() {
        final String testFilePath = UrlUtils.getIsolatedTestFilePath(
                "chrome/test/data/android/thumbnail_provider/test_image_10x10.jpg");
        final int requiredSize = 20;

        final TestThumbnailRequest request = new TestThumbnailRequest(testFilePath, requiredSize);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            public void run() {
                mThumbnailProvider.getThumbnail(request);
            }
        });

        checkResult(10, 10, testFilePath, request);
    }

    @Test
    @MediumTest
    @Feature({"Suggestions"})
    public void testBothThumbnailSidesEqualToRequiredSize() {
        final String testFilePath = UrlUtils.getIsolatedTestFilePath(
                "chrome/test/data/android/thumbnail_provider/test_image_10x10.jpg");
        final int requiredSize = 10;

        final TestThumbnailRequest request = new TestThumbnailRequest(testFilePath, requiredSize);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            public void run() {
                mThumbnailProvider.getThumbnail(request);
            }
        });

        checkResult(requiredSize, requiredSize, testFilePath, request);
    }

    @Test
    @MediumTest
    @Feature({"Suggestions"})
    public void testBothThumbnailSidesBiggerThanRequiredSize() {
        final String testFilePath = UrlUtils.getIsolatedTestFilePath(
                "chrome/test/data/android/thumbnail_provider/test_image_20x20.jpg");
        final int requiredSize = 10;

        final TestThumbnailRequest request = new TestThumbnailRequest(testFilePath, requiredSize);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            public void run() {
                mThumbnailProvider.getThumbnail(request);
            }
        });

        checkResult(requiredSize, requiredSize, testFilePath, request);
    }

    @Test
    @MediumTest
    @Feature({"Suggestions"})
    public void testThumbnailWidthEqualToRequiredSize() {
        final String testFilePath = UrlUtils.getIsolatedTestFilePath(
                "chrome/test/data/android/thumbnail_provider/test_image_10x20.jpg");
        final int requiredSize = 10;

        final TestThumbnailRequest request = new TestThumbnailRequest(testFilePath, requiredSize);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            public void run() {
                mThumbnailProvider.getThumbnail(request);
            }
        });

        checkResult(requiredSize, 20, testFilePath, request);
    }

    @Test
    @MediumTest
    @Feature({"Suggestions"})
    public void testThumbnailHeightEqualToRequiredSize() {
        final String testFilePath = UrlUtils.getIsolatedTestFilePath(
                "chrome/test/data/android/thumbnail_provider/test_image_20x10.jpg");
        final int requiredSize = 10;

        final TestThumbnailRequest request = new TestThumbnailRequest(testFilePath, requiredSize);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            public void run() {
                mThumbnailProvider.getThumbnail(request);
            }
        });

        checkResult(20, requiredSize, testFilePath, request);
    }

    private void checkResult(int expectedWidth, int expectedHeight, String expectedFilePath,
            final TestThumbnailRequest request) {
        CriteriaHelper.pollUiThread(Criteria.equals(expectedWidth, new Callable<Integer>() {
            @Override
            public Integer call() throws Exception {
                return request.getRetrievedThumbnail().getWidth();
            }
        }));

        CriteriaHelper.pollUiThread(Criteria.equals(expectedHeight, new Callable<Integer>() {
            @Override
            public Integer call() throws Exception {
                return request.getRetrievedThumbnail().getHeight();
            }
        }));

        CriteriaHelper.pollUiThread(Criteria.equals(expectedFilePath, new Callable<String>() {
            @Override
            public String call() throws Exception {
                return request.getRetrievedFilePath();
            }
        }));
    }

    private void clearThumbnailCache() {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                ThumbnailProviderImpl.clearCache();
            }
        });
    }

    private class TestThumbnailRequest implements ThumbnailRequest {
        private final String mTestFilePath;
        private final int mRequiredSize;
        private Bitmap mRetrievedThumbnail;
        private String mRetrievedFilePath;

        TestThumbnailRequest(String filepath, int requiredSize) {
            mTestFilePath = filepath;
            mRequiredSize = requiredSize;
        }

        @Override
        public String getFilePath() {
            return mTestFilePath;
        }

        @Override
        public void onThumbnailRetrieved(String filePath, Bitmap thumbnail) {
            mRetrievedThumbnail = thumbnail;
            mRetrievedFilePath = filePath;
        }

        @Override
        public int getIconSize() {
            return mRequiredSize;
        }

        Bitmap getRetrievedThumbnail() {
            return mRetrievedThumbnail;
        }

        String getRetrievedFilePath() {
            return mRetrievedFilePath;
        }
    }
}
