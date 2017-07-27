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
import org.chromium.net.CronetEngine;
import org.chromium.net.CronetTestRule;
import org.chromium.net.CronetTestRule.CompareDefaultWithCronet;
import org.chromium.net.CronetTestRule.OnlyRunCronetHttpURLConnection;
import org.chromium.net.NativeTestServer;
import org.chromium.net.NetworkException;

import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.ProtocolException;
import java.net.URL;

/**
 * Tests {@code getOutputStream} when {@code setChunkedStreamingMode} is enabled.
 * Tests annotated with {@code CompareDefaultWithCronet} will run once with the
 * default HttpURLConnection implementation and then with Cronet's
 * HttpURLConnection implementation. Tests annotated with
 * {@code OnlyRunCronetHttpURLConnection} only run Cronet's implementation.
 * See {@link CronetTestBase#runTest()} for details.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CronetChunkedOutputStreamTest {
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    private static final String UPLOAD_DATA_STRING = "Nifty upload data!";
    private static final byte[] UPLOAD_DATA = UPLOAD_DATA_STRING.getBytes();
    private static final int REPEAT_COUNT = 100000;

    @Before
    public void setUp() throws Exception {
        mTestRule.setStreamHandlerFactory(
                new CronetEngine.Builder(InstrumentationRegistry.getTargetContext()).build());
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
    @CompareDefaultWithCronet
    public void testGetOutputStreamAfterConnectionMade() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        Assert.assertEquals(200, connection.getResponseCode());
        try {
            connection.getOutputStream();
            Assert.fail();
        } catch (ProtocolException e) {
            // Expected.
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testWriteAfterReadingResponse() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        OutputStream out = connection.getOutputStream();
        Assert.assertEquals(200, connection.getResponseCode());
        try {
            out.write(UPLOAD_DATA);
            Assert.fail();
        } catch (IOException e) {
            // Expected.
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testWriteAfterRequestFailed() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        NativeTestServer.shutdownNativeTestServer();
        try {
            out.write(TestUtil.getLargeData());
            connection.getResponseCode();
            Assert.fail();
        } catch (IOException e) {
            if (!mTestRule.testingSystemHttpURLConnection()) {
                NetworkException requestException = (NetworkException) e;
                Assert.assertEquals(
                        NetworkException.ERROR_CONNECTION_REFUSED, requestException.getErrorCode());
            }
        }
        connection.disconnect();
        // Restarting server to run the test for a second time.
        Assert.assertTrue(
                NativeTestServer.startNativeTestServer(InstrumentationRegistry.getTargetContext()));
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testGetResponseAfterWriteFailed() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        NativeTestServer.shutdownNativeTestServer();
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        // Set 1 byte as chunk size so internally Cronet will try upload when
        // 1 byte is filled.
        connection.setChunkedStreamingMode(1);
        try {
            OutputStream out = connection.getOutputStream();
            out.write(1);
            out.write(1);
            // Forces OutputStream implementation to flush. crbug.com/653072
            out.flush();
            // System's implementation is flaky see crbug.com/653072.
            if (!mTestRule.testingSystemHttpURLConnection()) {
                Assert.fail();
            }
        } catch (IOException e) {
            if (!mTestRule.testingSystemHttpURLConnection()) {
                NetworkException requestException = (NetworkException) e;
                Assert.assertEquals(
                        NetworkException.ERROR_CONNECTION_REFUSED, requestException.getErrorCode());
            }
        }
        // Make sure IOException is reported again when trying to read response
        // from the connection.
        try {
            connection.getResponseCode();
            Assert.fail();
        } catch (IOException e) {
            // Expected.
            if (!mTestRule.testingSystemHttpURLConnection()) {
                NetworkException requestException = (NetworkException) e;
                Assert.assertEquals(
                        NetworkException.ERROR_CONNECTION_REFUSED, requestException.getErrorCode());
            }
        }
        // Restarting server to run the test for a second time.
        Assert.assertTrue(
                NativeTestServer.startNativeTestServer(InstrumentationRegistry.getTargetContext()));
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPost() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        Assert.assertEquals(200, connection.getResponseCode());
        Assert.assertEquals("OK", connection.getResponseMessage());
        Assert.assertEquals(UPLOAD_DATA_STRING, TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testTransferEncodingHeaderSet() throws Exception {
        URL url = new URL(NativeTestServer.getEchoHeaderURL("Transfer-Encoding"));
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        Assert.assertEquals(200, connection.getResponseCode());
        Assert.assertEquals("OK", connection.getResponseMessage());
        Assert.assertEquals("chunked", TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostOneMassiveWrite() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        OutputStream out = connection.getOutputStream();
        byte[] largeData = TestUtil.getLargeData();
        out.write(largeData);
        Assert.assertEquals(200, connection.getResponseCode());
        Assert.assertEquals("OK", connection.getResponseMessage());
        TestUtil.checkLargeData(TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWriteOneByte() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        OutputStream out = connection.getOutputStream();
        for (int i = 0; i < UPLOAD_DATA.length; i++) {
            out.write(UPLOAD_DATA[i]);
        }
        Assert.assertEquals(200, connection.getResponseCode());
        Assert.assertEquals("OK", connection.getResponseMessage());
        Assert.assertEquals(UPLOAD_DATA_STRING, TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostOneMassiveWriteWriteOneByte() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setChunkedStreamingMode(0);
        OutputStream out = connection.getOutputStream();
        byte[] largeData = TestUtil.getLargeData();
        for (int i = 0; i < largeData.length; i++) {
            out.write(largeData[i]);
        }
        Assert.assertEquals(200, connection.getResponseCode());
        Assert.assertEquals("OK", connection.getResponseMessage());
        TestUtil.checkLargeData(TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWholeNumberOfChunks() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        int totalSize = UPLOAD_DATA.length * REPEAT_COUNT;
        int chunkSize = 18000;
        // Ensure total data size is a multiple of chunk size, so no partial
        // chunks will be used.
        Assert.assertEquals(0, totalSize % chunkSize);
        connection.setChunkedStreamingMode(chunkSize);
        OutputStream out = connection.getOutputStream();
        byte[] largeData = TestUtil.getLargeData();
        out.write(largeData);
        Assert.assertEquals(200, connection.getResponseCode());
        Assert.assertEquals("OK", connection.getResponseMessage());
        TestUtil.checkLargeData(TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunCronetHttpURLConnection
    // Regression testing for crbug.com/618872.
    public void testOneMassiveWriteLargerThanInternalBuffer() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        // Use a super big chunk size so that it exceeds the UploadDataProvider
        // read buffer size.
        byte[] largeData = TestUtil.getLargeData();
        connection.setChunkedStreamingMode(largeData.length);
        OutputStream out = connection.getOutputStream();
        out.write(largeData);
        Assert.assertEquals(200, connection.getResponseCode());
        TestUtil.checkLargeData(TestUtil.getResponseAsString(connection));
        connection.disconnect();
    }
}
