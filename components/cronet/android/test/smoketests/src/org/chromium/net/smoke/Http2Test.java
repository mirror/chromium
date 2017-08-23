// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.smoke;

import android.support.test.filters.SmallTest;

import org.chromium.net.UrlRequest;
import org.junit.Test;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.junit.runner.RunWith;
import android.support.test.InstrumentationRegistry;
import org.junit.Assert;
import org.junit.After;
import org.junit.Before;

/**
 * HTTP2 Tests.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class Http2Test {
    private TestSupport.TestServer mServer;

    @Before
    public void setUp() throws Exception {
        mServer = mTestSupport.createTestServer(
                InstrumentationRegistry.getContext(), TestSupport.Protocol.HTTP2);
    }

    @After
    public void tearDown() throws Exception {
        mServer.shutdown();
    }

    // Test that HTTP/2 is enabled by default but QUIC is not.
    @Test
    @SmallTest
    public void testHttp2() throws Exception {
        mTestSupport.installMockCertVerifierForTesting(mCronetEngineBuilder);
        initCronetEngine();
        Assert.assertTrue(mServer.start());
        SmokeTestRequestCallback callback = new SmokeTestRequestCallback();
        UrlRequest.Builder requestBuilder = mCronetEngine.newUrlRequestBuilder(
                mServer.getSuccessURL(), callback, callback.getExecutor());
        requestBuilder.build().start();
        callback.blockForDone();

        assertSuccessfulNonEmptyResponse(callback, mServer.getSuccessURL());
        Assert.assertEquals("h2", callback.getResponseInfo().getNegotiatedProtocol());
    }
}
