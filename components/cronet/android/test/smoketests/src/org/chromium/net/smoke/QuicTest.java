// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.smoke;

import android.support.test.filters.SmallTest;

import org.json.JSONObject;

import static org.chromium.net.smoke.TestSupport.Protocol.QUIC;

import org.chromium.net.UrlRequest;

import java.net.URL;
import org.junit.Test;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.junit.runner.RunWith;
import android.support.test.InstrumentationRegistry;
import org.junit.Assert;
import org.junit.After;
import org.junit.Before;

/**
 * QUIC Tests.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class QuicTest {
    private TestSupport.TestServer mServer;

    @Before
    public void setUp() throws Exception {
        mServer = mTestSupport.createTestServer(InstrumentationRegistry.getContext(), QUIC);
    }

    @After
    public void tearDown() throws Exception {
        mServer.shutdown();
    }

    @Test
    @SmallTest
    public void testQuic() throws Exception {
        Assert.assertTrue(mServer.start());
        final String urlString = mServer.getSuccessURL();
        final URL url = new URL(urlString);

        mCronetEngineBuilder.enableQuic(true);
        mCronetEngineBuilder.addQuicHint(url.getHost(), url.getPort(), url.getPort());
        mTestSupport.installMockCertVerifierForTesting(mCronetEngineBuilder);

        JSONObject quicParams = new JSONObject();
        JSONObject experimentalOptions = new JSONObject().put("QUIC", quicParams);
        mTestSupport.addHostResolverRules(experimentalOptions);
        mCronetEngineBuilder.setExperimentalOptions(experimentalOptions.toString());

        initCronetEngine();

        // QUIC is not guaranteed to win the race, so try multiple times.
        boolean quicNegotiated = false;

        for (int i = 0; i < 5; i++) {
            SmokeTestRequestCallback callback = new SmokeTestRequestCallback();
            UrlRequest.Builder requestBuilder =
                    mCronetEngine.newUrlRequestBuilder(urlString, callback, callback.getExecutor());
            requestBuilder.build().start();
            callback.blockForDone();
            assertSuccessfulNonEmptyResponse(callback, urlString);
            if (callback.getResponseInfo().getNegotiatedProtocol().startsWith("http/2+quic/")) {
                quicNegotiated = true;
                break;
            }
        }
        Assert.assertTrue(quicNegotiated);
    }
}
