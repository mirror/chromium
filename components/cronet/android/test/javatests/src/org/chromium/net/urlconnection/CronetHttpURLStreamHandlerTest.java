// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.urlconnection;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.net.CronetTestRule;
import org.chromium.net.CronetTestRule.CronetTestFramework;
import org.chromium.net.NativeTestServer;

import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.Proxy;
import java.net.URL;

/**
 * Tests for CronetHttpURLStreamHandler class.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CronetHttpURLStreamHandlerTest {
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    private CronetTestFramework mTestFramework;

    @Before
    public void setUp() throws Exception {
        mTestFramework = mTestRule.startCronetTestFramework();
        Assert.assertTrue(
                NativeTestServer.startNativeTestServer(InstrumentationRegistry.getTargetContext()));
    }

    @After
    public void tearDown() throws Exception {
        NativeTestServer.shutdownNativeTestServer();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testOpenConnectionHttp() throws Exception {
        URL url = new URL(NativeTestServer.getEchoMethodURL());
        CronetHttpURLStreamHandler streamHandler =
                new CronetHttpURLStreamHandler(mTestFramework.mCronetEngine);
        HttpURLConnection connection =
                (HttpURLConnection) streamHandler.openConnection(url);
        Assert.assertEquals(200, connection.getResponseCode());
        Assert.assertEquals("OK", connection.getResponseMessage());
        Assert.assertEquals("GET", TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testOpenConnectionHttps() throws Exception {
        URL url = new URL("https://example.com");
        CronetHttpURLStreamHandler streamHandler =
                new CronetHttpURLStreamHandler(mTestFramework.mCronetEngine);
        HttpURLConnection connection =
                (HttpURLConnection) streamHandler.openConnection(url);
        Assert.assertNotNull(connection);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testOpenConnectionProtocolNotSupported() throws Exception {
        URL url = new URL("ftp://example.com");
        CronetHttpURLStreamHandler streamHandler =
                new CronetHttpURLStreamHandler(mTestFramework.mCronetEngine);
        try {
            streamHandler.openConnection(url);
            Assert.fail();
        } catch (UnsupportedOperationException e) {
            Assert.assertEquals("Unexpected protocol:ftp", e.getMessage());
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testOpenConnectionWithProxy() throws Exception {
        URL url = new URL(NativeTestServer.getEchoMethodURL());
        CronetHttpURLStreamHandler streamHandler =
                new CronetHttpURLStreamHandler(mTestFramework.mCronetEngine);
        Proxy proxy = new Proxy(Proxy.Type.HTTP,
                new InetSocketAddress("127.0.0.1", 8080));
        try {
            streamHandler.openConnection(url, proxy);
            Assert.fail();
        } catch (UnsupportedOperationException e) {
            // Expected.
        }
    }
}
