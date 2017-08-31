// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.support.test.filters.MediumTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_shell_apk.ContentShellActivityTestRule;

import java.util.concurrent.TimeoutException;

/**
 * Class which provides test coverage for Tap Disambiguation UI.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@RetryOnFailure
public class TapDisambiguationTest {
    private static final String TARGET_NODE_ID = "target";

    @Rule
    public ContentShellActivityTestRule mActivityTestRule = new ContentShellActivityTestRule();

    /**
     * Creates a webpage that has a couple links next to one another with a zero-width node between
     * them. Clicking on the zero-width node should trigger the popup zoomer to appear.
     */
    private String generateTestUrl() {
        final StringBuilder testUrl = new StringBuilder();
        testUrl.append("<html><body>");
        testUrl.append("<a href=\"javascript:void(0);\">A</a>");
        testUrl.append("<a id=\"" + TARGET_NODE_ID + "\"></a>");
        testUrl.append("<a href=\"javascript:void(0);\">Z</a>");
        testUrl.append("</body></html>");
        return UrlUtils.encodeHtmlDataUri(testUrl.toString());
    }

    public TapDisambiguationTest() {}

    /**
     * Tests that shows tap disambiguation triggers zooming in.
     */
    @Test
    @MediumTest
    @Feature({"Browser"})
    public void testTapDisambiguationZoom() throws InterruptedException, TimeoutException {
        mActivityTestRule.launchContentShellWithUrl(generateTestUrl());
        mActivityTestRule.waitForActiveShellToBeDoneLoading();

        final ContentViewCore cvc = mActivityTestRule.getContentViewCore();
        final float initialZoomFactor = cvc.getPageScaleFactor();

        // Once clicked, the page should be zoomed in with updated scale factor.
        DOMUtils.clickNode(cvc, TARGET_NODE_ID);
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return cvc.getPageScaleFactor() > initialZoomFactor;
            }
        });
    }
}
