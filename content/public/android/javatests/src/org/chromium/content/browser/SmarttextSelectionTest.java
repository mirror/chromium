// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

// import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
// import org.junit.runner.RunWith;

import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.content.browser.test.ContentJUnit4ClassRunner;
import org.chromium.content_shell_apk.ContentShellActivityTestRule;

/**
 * Integration tests for text selection-related behavior.
 */
@RunWith(ContentJUnit4ClassRunner.class)
public class ContentViewCoreSelectionTest {
    @Rule
    public ContentShellActivityTestRule mActivityTestRule = new ContentShellActivityTestRule();

    private static final String DATA_URL =
            UrlUtils.encodeHtmlDataUri("<html><body><p id='select'>415-555-6789</p></body></html>");

    private ContentViewCore mContentViewCore;
    private SelectionPopupController mSelectionPopupController;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.launchContentShellWithUrl(DATA_URL);
        mActivityTestRule.waitForActiveShellToBeDoneLoading();

        mContentViewCore = mActivityTestRule.getContentViewCore();
        mSelectionPopupController = mContentViewCore.getSelectionPopupControllerForTesting();
    }

    @Test
    @SmallTest
    @Feature({"SmartTextSelection"})
    public void testSuggestIsCalled() throws Exception {}
}
