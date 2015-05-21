// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.test.FlakyTest;

import org.chromium.chrome.browser.ssl.ConnectionSecurityHelperSecurityLevel;
import org.chromium.content_public.common.ScreenOrientationValues;

/**
 * Tests whether the URL bar updates itself properly.
 */
public class WebappUrlBarTest extends WebappActivityTestBase {
    private static final String WEBAPP_URL = "http://originalwebsite.com";
    private WebappUrlBar mUrlBar;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        WebappInfo mockInfo = WebappInfo.create(WEBAPP_ID, WEBAPP_URL, null, null,
                ScreenOrientationValues.DEFAULT);
        getActivity().getWebappInfo().copy(mockInfo);
        mUrlBar = getActivity().getUrlBarForTests();
    }

    /*
    @UiThreadTest
    @MediumTest
    @Feature({"Webapps"})
    crbug/407332
    */
    @FlakyTest
    public void testUrlDisplay() {
        final String scheme = "somescheme://";
        final String host = "lorem.com";
        final String path = "/stuff/and/things.html";
        final String url = scheme + host + path;
        final String urlExpectedWhenIconNotShown = scheme + host;
        final String urlExpectedWhenIconShown = host;
        final int[] securityLevels = {
            ConnectionSecurityHelperSecurityLevel.NONE,
            ConnectionSecurityHelperSecurityLevel.EV_SECURE,
            ConnectionSecurityHelperSecurityLevel.SECURE,
            ConnectionSecurityHelperSecurityLevel.SECURITY_WARNING,
            ConnectionSecurityHelperSecurityLevel.SECURITY_POLICY_WARNING,
            ConnectionSecurityHelperSecurityLevel.SECURITY_ERROR };

        for (int i : securityLevels) {
            // http://crbug.com/297249
            if (i == ConnectionSecurityHelperSecurityLevel.SECURITY_POLICY_WARNING) continue;
            mUrlBar.update(url, i);

            int iconResource = mUrlBar.getCurrentIconResourceForTests();
            if (iconResource == 0) {
                assertEquals(
                        urlExpectedWhenIconNotShown, mUrlBar.getDisplayedUrlForTests().toString());
            } else {
                assertEquals(
                        urlExpectedWhenIconShown, mUrlBar.getDisplayedUrlForTests().toString());
            }
        }
    }
}
