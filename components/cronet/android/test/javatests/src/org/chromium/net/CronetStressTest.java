// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import static org.chromium.net.CronetTestRule.getContext;

import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.net.CronetTestRule.OnlyRunNativeCronet;

/**
 * Tests that making a large number of requests.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CronetStressTest {
    @SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD")
    @Rule
    public final CronetTestRule mTestRule = new CronetTestRule();

    private CronetEngine mCronetEngine;

    @Before
    protected void setUp() throws Exception {
        ExperimentalCronetEngine.Builder builder =
                new ExperimentalCronetEngine.Builder(getContext());
        mCronetEngine = builder.build();
        assertTrue(NativeTestServer.startNativeTestServer(getContext()));
    }

    @After
    protected void tearDown() throws Exception {
        NativeTestServer.shutdownNativeTestServer();
        mCronetEngine.shutdown();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testLargeNumberOfUploads() throws Exception {
        final int kNumRequest = 1000;
        final int kNumRequestHeaders = 100;
        final int kNumUploadBytes = 1000;
        for (int i = 0; i < kNumRequest; i++) {
            TestUrlRequestCallback callback = new TestUrlRequestCallback();
            UrlRequest.Builder builder = mCronetEngine.newUrlRequestBuilder(
                    NativeTestServer.getEchoAllHeadersURL(), callback, callback.getExecutor());
            for (int j = 0; j < kNumRequestHeaders; j++) {
                builder.addHeader("header" + j, Integer.toString(j));
            }
            builder.addHeader("content-type", "useless/string");
            byte[] b = new byte[kNumUploadBytes];
            builder.setUploadDataProvider(
                    UploadDataProviders.create(b, 0, kNumUploadBytes), callback.getExecutor());
            builder.build().start();
            callback.blockForDone();
            assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
        }
    }
}
