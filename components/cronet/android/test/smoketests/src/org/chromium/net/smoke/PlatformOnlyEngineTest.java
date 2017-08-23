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
 * Tests scenario when an app doesn't contain the native Cronet implementation.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class PlatformOnlyEngineTest {
    private String mURL;
    private TestSupport.TestServer mServer;

    @Before
    public void setUp() throws Exception {
        // Java-only implementation of the Cronet engine only supports Http/1 protocol.
        mServer = mTestSupport.createTestServer(
                InstrumentationRegistry.getContext(), TestSupport.Protocol.HTTP1);
        Assert.assertTrue(mServer.start());
        mURL = mServer.getSuccessURL();
    }

    @After
    public void tearDown() throws Exception {
        mServer.shutdown();
    }

    /**
     * Test a successful response when a request is sent by the Java Cronet Engine.
     */
    @Test
    @SmallTest
    public void testSuccessfulResponse() {
        initCronetEngine();
        assertJavaEngine(mCronetEngine);
        SmokeTestRequestCallback callback = new SmokeTestRequestCallback();
        UrlRequest.Builder requestBuilder =
                mCronetEngine.newUrlRequestBuilder(mURL, callback, callback.getExecutor());
        requestBuilder.build().start();
        callback.blockForDone();
        assertSuccessfulNonEmptyResponse(callback, mURL);
    }
}
