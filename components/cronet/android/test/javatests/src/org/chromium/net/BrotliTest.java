// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import static org.chromium.net.CronetTestRule.SERVER_CERT_PEM;
import static org.chromium.net.CronetTestRule.SERVER_KEY_PKCS8_PEM;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.net.CronetTestRule.OnlyRunNativeCronet;
import org.chromium.net.CronetTestRule.RequiresMinApi;

/**
 * Simple test for Brotli support.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@RequiresMinApi(5) // Brotli support added in API version 5: crrev.com/465216
public class BrotliTest {
    @SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD")
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    private CronetEngine mCronetEngine;

    @Before
    public void setUp() throws Exception {
        TestFilesInstaller.installIfNeeded(InstrumentationRegistry.getTargetContext());
        Assert.assertTrue(Http2TestServer.startHttp2TestServer(
                InstrumentationRegistry.getTargetContext(), SERVER_CERT_PEM, SERVER_KEY_PKCS8_PEM));
    }

    @After
    public void tearDown() throws Exception {
        Assert.assertTrue(Http2TestServer.shutdownHttp2TestServer());
        if (mCronetEngine != null) {
            mCronetEngine.shutdown();
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testBrotliAdvertised() throws Exception {
        ExperimentalCronetEngine.Builder builder =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getTargetContext());
        builder.enableBrotli(true);
        CronetTestUtil.setMockCertVerifierForTesting(
                builder, QuicTestServer.createMockCertVerifier());
        mCronetEngine = builder.build();
        String url = Http2TestServer.getEchoAllHeadersUrl();
        TestUrlRequestCallback callback = startAndWaitForComplete(url);
        Assert.assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
        Assert.assertTrue(
                callback.mResponseAsString.contains("accept-encoding: gzip, deflate, br"));
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testBrotliNotAdvertised() throws Exception {
        ExperimentalCronetEngine.Builder builder =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getTargetContext());
        CronetTestUtil.setMockCertVerifierForTesting(
                builder, QuicTestServer.createMockCertVerifier());
        mCronetEngine = builder.build();
        String url = Http2TestServer.getEchoAllHeadersUrl();
        TestUrlRequestCallback callback = startAndWaitForComplete(url);
        Assert.assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
        Assert.assertFalse(callback.mResponseAsString.contains("br"));
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testBrotliDecoded() throws Exception {
        ExperimentalCronetEngine.Builder builder =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getTargetContext());
        builder.enableBrotli(true);
        CronetTestUtil.setMockCertVerifierForTesting(
                builder, QuicTestServer.createMockCertVerifier());
        mCronetEngine = builder.build();
        String url = Http2TestServer.getServeSimpleBrotliResponse();
        TestUrlRequestCallback callback = startAndWaitForComplete(url);
        Assert.assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
        String expectedResponse = "The quick brown fox jumps over the lazy dog";
        Assert.assertEquals(expectedResponse, callback.mResponseAsString);
        Assert.assertEquals(
                callback.mResponseInfo.getAllHeaders().get("content-encoding").get(0), "br");
    }

    private TestUrlRequestCallback startAndWaitForComplete(String url) {
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                mCronetEngine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        builder.build().start();
        callback.blockForDone();
        return callback;
    }
}
