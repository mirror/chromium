// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.Activity;

import org.chromium.content.browser.test.util.TestCallbackHelperContainer;
import org.chromium.content_shell_apk.ContentShellActivity;
import org.chromium.content_shell_apk.ContentShellTestBase;
import org.chromium.content_shell_apk.ContentShellTestCommon.TestCommonCallback;

/**
 * Base class for content detection test suites.
 */
public class ContentDetectionTestBase
        extends ContentShellTestBase implements TestCommonCallback<ContentShellActivity> {
    private final ContentDetectionTestCommon mTestCommon = new ContentDetectionTestCommon(this);

    /**
     * Returns the TestCallbackHelperContainer associated with this ContentView,
     * or creates it lazily.
     */
    protected TestCallbackHelperContainer getTestCallbackHelperContainer() {
        return mTestCommon.getTestCallbackHelperContainer();
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTestCommon.createTestContentIntentHandler();
    }

    @SuppressWarnings("deprecation")
    @Override
    protected void setActivity(Activity activity) {
        super.setActivity(activity);
        mTestCommon.setContentHandler();
    }

    /**
     * Checks if the provided test url is the current url in the content view.
     * @param testUrl Test url to check.
     * @return true if the test url is the current one, false otherwise.
     */
    protected boolean isCurrentTestUrl(String testUrl) {
        return mTestCommon.isCurrentTestUrl(testUrl);
    }

    /**
     * Encodes the provided content string into an escaped url as intents do.
     * @param content Content to escape into a url.
     * @return Escaped url.
     */
    public static String urlForContent(String content) {
        return ContentDetectionTestCommon.urlForContent(content);
    }

    /**
     * Scrolls to the node with the provided id, taps on it and waits for an intent to come.
     * @param id Id of the node to scroll and tap.
     * @return The content url of the received intent or null if none.
     */
    protected String scrollAndTapExpectingIntent(String id) throws Throwable {
        return mTestCommon.scrollAndTapExpectingIntent(id);
    }

    /**
     * Scrolls to the node with the provided id, taps on it and waits for a new page load to finish.
     * Useful when tapping on links that take to other pages.
     * @param id Id of the node to scroll and tap.
     */
    protected void scrollAndTapNavigatingOut(String id) throws Throwable {
        mTestCommon.scrollAndTapNavigatingOut(id);
    }
}