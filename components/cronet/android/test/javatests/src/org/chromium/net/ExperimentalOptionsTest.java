// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import android.support.test.filters.MediumTest;

import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.test.util.Feature;
import org.chromium.net.impl.CronetUrlRequestContext;
import org.chromium.net.test.EmbeddedTestServer;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URL;

/**
 * Tests for experimental options.
 */
@JNINamespace("cronet")
public class ExperimentalOptionsTest extends CronetTestBase {
    private static final String TAG = ExperimentalOptionsTest.class.getSimpleName();
    private ExperimentalCronetEngine.Builder mBuilder;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mBuilder = new ExperimentalCronetEngine.Builder(getContext());
        CronetTestUtil.setMockCertVerifierForTesting(
                mBuilder, QuicTestServer.createMockCertVerifier());
        assertTrue(Http2TestServer.startHttp2TestServer(
                getContext(), SERVER_CERT_PEM, SERVER_KEY_PKCS8_PEM));
    }

    @Override
    protected void tearDown() throws Exception {
        assertTrue(Http2TestServer.shutdownHttp2TestServer());
        super.tearDown();
    }

    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that NetLog writes effective experimental options to NetLog.
    public void testNetLog() throws Exception {
        File directory = new File(PathUtils.getDataDirectory());
        File logfile = File.createTempFile("cronet", "json", directory);
        JSONObject hostResolverParams = CronetTestUtil.generateHostResolverRules();
        JSONObject experimentalOptions =
                new JSONObject().put("HostResolverRules", hostResolverParams);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());

        CronetEngine cronetEngine = mBuilder.build();
        cronetEngine.startNetLogToFile(logfile.getPath(), false);
        String url = Http2TestServer.getEchoMethodUrl();
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                cronetEngine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
        assertEquals("GET", callback.mResponseAsString);
        cronetEngine.stopNetLog();
        assertFileContainsString(logfile, "HostResolverRules");
        assertTrue(logfile.delete());
        assertFalse(logfile.exists());
        cronetEngine.shutdown();
    }

    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testSetSSLKeyLogFile() throws Exception {
        String url = Http2TestServer.getEchoMethodUrl();
        File dir = new File(PathUtils.getDataDirectory());
        File file = File.createTempFile("ssl_key_log_file", "", dir);

        JSONObject experimentalOptions = new JSONObject().put("ssl_key_log_file", file.getPath());
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetEngine cronetEngine = mBuilder.build();

        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                cronetEngine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
        assertEquals("GET", callback.mResponseAsString);

        assertFileContainsString(file, "CLIENT_RANDOM");
        assertTrue(file.delete());
        assertFalse(file.exists());
        cronetEngine.shutdown();
    }

    // Helper method to assert that file contains content. It retries 5 times
    // with a 100ms interval.
    private void assertFileContainsString(File file, String content) throws Exception {
        boolean contains = false;
        for (int i = 0; i < 5; i++) {
            contains = fileContainsString(file, content);
            if (contains) break;
            Log.i(TAG, "Retrying...");
            Thread.sleep(100);
        }
        assertTrue("file content doesn't match", contains);
    }

    // Returns whether a file contains a particular string.
    private boolean fileContainsString(File file, String content) throws IOException {
        FileInputStream fileInputStream = null;
        Log.i(TAG, "looking for [%s] in %s", content, file.getName());
        try {
            fileInputStream = new FileInputStream(file);
            byte[] data = new byte[(int) file.length()];
            fileInputStream.read(data);
            String actual = new String(data, "UTF-8");
            boolean contains = actual.contains(content);
            //            if (!contains) {
            Log.i(TAG, "file content [%s]", actual);
            //            }
            return contains;
        } catch (FileNotFoundException e) {
            // Ignored this exception since the file will only be created when updates are
            // flushed to the disk.
            Log.i(TAG, "file not found");
        } finally {
            if (fileInputStream != null) {
                fileInputStream.close();
            }
        }
        return false;
    }

    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence00() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence01() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence02() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence03() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence04() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence05() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence06() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence07() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence08() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence09() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence10() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence11() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence12() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence13() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence14() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence15() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence16() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence17() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence18() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence19() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence20() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence21() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence22() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence23() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence24() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence98() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence101() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence25() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence26() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence27() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence28() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence29() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence30() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence31() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence32() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence33() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence34() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence99() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence35() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence36() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence37() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence38() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence39() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence40() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence41() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence42() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence43() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence44() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence45() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence46() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence47() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence48() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence49() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence50() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence51() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence52() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence53() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence54() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence55() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence56() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence57() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence58() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence59() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence100() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence60() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence61() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence62() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence63() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence64() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence65() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence66() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence67() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence68() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence69() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence70() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence71() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence72() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence73() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence74() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence75() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence76() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence77() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence78() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence79() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence80() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence81() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence82() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence83() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence84() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence85() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence86() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence87() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence88() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence89() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence90() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence91() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence92() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence93() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence94() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence95() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence96() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }
    @MediumTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that basic Cronet functionality works when host cache persistence is enabled, and that
    // persistence works.
    public void testHostCachePersistence97() throws Exception {
        EmbeddedTestServer testServer = EmbeddedTestServer.createAndStartServer(getContext());

        String realUrl = testServer.getURL("/echo?status=200");
        URL javaUrl = new URL(realUrl);
        String realHost = javaUrl.getHost();
        int realPort = javaUrl.getPort();
        String testHost = "host-cache-test-host";
        String testUrl = new URL("http", testHost, realPort, javaUrl.getPath()).toString();

        mBuilder.setStoragePath(getTestStorage(getContext()));

        // Set a short delay so the pref gets written quickly.
        JSONObject staleDns = new JSONObject()
                                      .put("enable", true)
                                      .put("delay_ms", 0)
                                      .put("allow_other_network", true)
                                      .put("persist_to_disk", true)
                                      .put("persist_delay_ms", 0);
        JSONObject experimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        mBuilder.setExperimentalOptions(experimentalOptions.toString());
        CronetUrlRequestContext context = (CronetUrlRequestContext) mBuilder.build();

        // Create a HostCache entry for "host-cache-test-host".
        nativeWriteToHostCache(context.getUrlRequestContextAdapter(), realHost);

        // Do a request for the test URL to make sure it's cached.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder =
                context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        // First wait a little longer for the write to prefs to go through. Then shut down the
        // context, persisting contents to disk, and build a new one.
        Thread.sleep(100);
        context.shutdown();
        File file = new File(getTestStorage(getContext()) + "/prefs/local_prefs.json");
        assertFileContainsString(file, "host_cache");
        context = (CronetUrlRequestContext) mBuilder.build();

        // Use the test URL without creating a new cache entry first. It should use the persisted
        // one.
        callback = new TestUrlRequestCallback();
        builder = context.newUrlRequestBuilder(testUrl, callback, callback.getExecutor());
        urlRequest = builder.build();
        urlRequest.start();
        callback.blockForDone();
        assertNull(callback.mError);
        assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }

    // Sets a host cache entry with hostname "host-cache-test-host" and an AddressList containing
    // the provided address.
    private static native void nativeWriteToHostCache(long adapter, String address);
}
