// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.cronet_test_apk;

import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.test.util.Feature;
import org.chromium.net.HttpUrlRequest;

import java.util.HashMap;
import java.util.List;

/**
 * Tests that use mock URLRequestJobs to simulate URL requests.
 */
public class MockUrlRequestJobTest extends CronetTestBase {
    private static final String TAG = "MockURLRequestJobTest";

    private CronetTestActivity mActivity;
    private MockUrlRequestJobFactory mMockUrlRequestJobFactory;

    // Helper function to create a HttpUrlRequest with the specified url.
    private TestHttpUrlRequestListener createRequestAndWaitForComplete(
            String url, boolean disableRedirects) {
        HashMap<String, String> headers = new HashMap<String, String>();
        TestHttpUrlRequestListener listener = new TestHttpUrlRequestListener();

        HttpUrlRequest request = mActivity.mRequestFactory.createRequest(
                url,
                HttpUrlRequest.REQUEST_PRIORITY_MEDIUM,
                headers,
                listener);
        if (disableRedirects) {
            request.disableRedirects();
        }
        request.start();
        listener.blockForComplete();
        return listener;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = launchCronetTestApp();
        mMockUrlRequestJobFactory = new MockUrlRequestJobFactory(
                getInstrumentation().getTargetContext());
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testSuccessURLRequest() throws Exception {
        TestHttpUrlRequestListener listener = createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.SUCCESS_URL, false);
        assertEquals(MockUrlRequestJobFactory.SUCCESS_URL, listener.mUrl);
        assertEquals(200, listener.mHttpStatusCode);
        assertEquals("OK", listener.mHttpStatusText);
        assertEquals("this is a text file\n",
                new String(listener.mResponseAsBytes));
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testRedirectURLRequest() throws Exception {
        TestHttpUrlRequestListener listener = createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.REDIRECT_URL, false);

        // Currently Cronet does not expose the url after redirect.
        assertEquals(MockUrlRequestJobFactory.REDIRECT_URL, listener.mUrl);
        assertEquals(200, listener.mHttpStatusCode);
        assertEquals("OK", listener.mHttpStatusText);
        // Expect that the request is redirected to success.txt.
        assertEquals("this is a text file\n",
                new String(listener.mResponseAsBytes));
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testNotFoundURLRequest() throws Exception {
        TestHttpUrlRequestListener listener = createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.NOTFOUND_URL, false);
        assertEquals(MockUrlRequestJobFactory.NOTFOUND_URL, listener.mUrl);
        assertEquals(404, listener.mHttpStatusCode);
        assertEquals("Not Found", listener.mHttpStatusText);
        assertEquals(
                "<!DOCTYPE html>\n<html>\n<head>\n<title>Not found</title>\n"
                + "<p>Test page loaded.</p>\n</head>\n</html>\n",
                new String(listener.mResponseAsBytes));
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testFailedURLRequest() throws Exception {
        TestHttpUrlRequestListener listener = createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.FAILED_URL, false);

        assertEquals(MockUrlRequestJobFactory.FAILED_URL, listener.mUrl);
        assertEquals(null, listener.mHttpStatusText);
        assertEquals(0, listener.mHttpStatusCode);
    }

    @SmallTest
    @Feature({"Cronet"})
    // Test that redirect can be disabled for a request.
    public void testDisableRedirects() throws Exception {
        TestHttpUrlRequestListener listener = createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.REDIRECT_URL, true);
        // Currently Cronet does not expose the url after redirect.
        assertEquals(MockUrlRequestJobFactory.REDIRECT_URL, listener.mUrl);
        assertEquals(302, listener.mHttpStatusCode);
        // Expect that the request is not redirected to success.txt.
        assertNotNull(listener.mResponseHeaders);
        List<String> entry = listener.mResponseHeaders.get("redirect-header");
        assertEquals(1, entry.size());
        assertEquals("header-value", entry.get(0));
        List<String> location = listener.mResponseHeaders.get("Location");
        assertEquals(1, location.size());
        assertEquals("http://mock.http/success.txt", location.get(0));
        assertEquals("Request failed because there were too many redirects or "
                         + "redirects have been disabled",
                     listener.mException.getMessage());
    }
}
