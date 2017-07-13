// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import static org.chromium.net.CronetEngine.Builder.HTTP_CACHE_IN_MEMORY;

import android.content.Context;
import android.content.ContextWrapper;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.json.JSONObject;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.FileUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.net.CronetTestRule.CronetTestFramework;
import org.chromium.net.CronetTestRule.OnlyRunNativeCronet;
import org.chromium.net.CronetTestRule.RequiresMinApi;
import org.chromium.net.TestUrlRequestCallback.ResponseStep;
import org.chromium.net.impl.CronetEngineBuilderImpl;
import org.chromium.net.impl.CronetLibraryLoader;
import org.chromium.net.impl.CronetUrlRequestContext;
import org.chromium.net.test.EmbeddedTestServer;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.concurrent.Callable;
import java.util.concurrent.Executor;
import java.util.concurrent.FutureTask;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Test CronetEngine.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@JNINamespace("cronet")
public class CronetUrlRequestContextTest {
    @SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD")
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    private static final String TAG = CronetUrlRequestContextTest.class.getSimpleName();

    // URLs used for tests.
    private static final String MOCK_CRONET_TEST_FAILED_URL =
            "http://mock.failed.request/-2";
    private static final String MOCK_CRONET_TEST_SUCCESS_URL =
            "http://mock.http/success.txt";
    private static final int MAX_FILE_SIZE = 1000000000;
    private static final int NUM_EVENT_FILES = 10;

    private EmbeddedTestServer mTestServer;
    private String mUrl;
    private String mUrl404;
    private String mUrl500;

    @Before
    public void setUp() throws Exception {
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mUrl = mTestServer.getURL("/echo?status=200");
        mUrl404 = mTestServer.getURL("/echo?status=404");
        mUrl500 = mTestServer.getURL("/echo?status=500");
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
    }

    static class RequestThread extends Thread {
        public TestUrlRequestCallback mCallback;

        final String mUrl;
        final ConditionVariable mRunBlocker;

        public RequestThread(String url, ConditionVariable runBlocker) {
            mUrl = url;
            mRunBlocker = runBlocker;
        }

        @Override
        public void run() {
            mRunBlocker.block();
            CronetEngine cronetEngine =
                    new CronetEngine.Builder(InstrumentationRegistry.getContext()).build();
            mCallback = new TestUrlRequestCallback();
            UrlRequest.Builder urlRequestBuilder =
                    cronetEngine.newUrlRequestBuilder(mUrl, mCallback, mCallback.getExecutor());
            urlRequestBuilder.build().start();
            mCallback.blockForDone();
        }
    }

    /**
     * Callback that shutdowns the request context when request has succeeded
     * or failed.
     */
    static class ShutdownTestUrlRequestCallback extends TestUrlRequestCallback {
        private final CronetEngine mCronetEngine;
        private final ConditionVariable mCallbackCompletionBlock = new ConditionVariable();

        ShutdownTestUrlRequestCallback(CronetEngine cronetEngine) {
            mCronetEngine = cronetEngine;
        }

        @Override
        public void onSucceeded(UrlRequest request, UrlResponseInfo info) {
            super.onSucceeded(request, info);
            mCronetEngine.shutdown();
            mCallbackCompletionBlock.open();
        }

        @Override
        public void onFailed(UrlRequest request, UrlResponseInfo info, CronetException error) {
            super.onFailed(request, info, error);
            mCronetEngine.shutdown();
            mCallbackCompletionBlock.open();
        }

        // Wait for request completion callback.
        void blockForCallbackToComplete() {
            mCallbackCompletionBlock.block();
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @SuppressWarnings("deprecation")
    public void testConfigUserAgent() throws Exception {
        String userAgentName = "User-Agent";
        String userAgentValue = "User-Agent-Value";
        ExperimentalCronetEngine.Builder cronetEngineBuilder =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getContext());
        if (mTestRule.testingJavaImpl()) {
            cronetEngineBuilder = mTestRule.createJavaEngineBuilder();
        }
        cronetEngineBuilder.setUserAgent(userAgentValue);
        final CronetEngine cronetEngine = cronetEngineBuilder.build();
        NativeTestServer.shutdownNativeTestServer(); // startNativeTestServer returns false if it's
        // already running
        Assert.assertTrue(
                NativeTestServer.startNativeTestServer(InstrumentationRegistry.getContext()));
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder = cronetEngine.newUrlRequestBuilder(
                NativeTestServer.getEchoHeaderURL(userAgentName), callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertEquals(userAgentValue, callback.mResponseAsString);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    // TODO: Remove the annotation after fixing http://crbug.com/637979 & http://crbug.com/637972
    @OnlyRunNativeCronet
    public void testShutdown() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        ShutdownTestUrlRequestCallback callback =
                new ShutdownTestUrlRequestCallback(testFramework.mCronetEngine);
        // Block callback when response starts to verify that shutdown fails
        // if there are active requests.
        callback.setAutoAdvance(false);
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = urlRequestBuilder.build();
        urlRequest.start();
        try {
            testFramework.mCronetEngine.shutdown();
            Assert.fail("Should throw an exception");
        } catch (Exception e) {
            Assert.assertEquals("Cannot shutdown with active requests.", e.getMessage());
        }

        callback.waitForNextStep();
        Assert.assertEquals(ResponseStep.ON_RESPONSE_STARTED, callback.mResponseStep);
        try {
            testFramework.mCronetEngine.shutdown();
            Assert.fail("Should throw an exception");
        } catch (Exception e) {
            Assert.assertEquals("Cannot shutdown with active requests.", e.getMessage());
        }
        callback.startNextRead(urlRequest);

        callback.waitForNextStep();
        Assert.assertEquals(ResponseStep.ON_READ_COMPLETED, callback.mResponseStep);
        try {
            testFramework.mCronetEngine.shutdown();
            Assert.fail("Should throw an exception");
        } catch (Exception e) {
            Assert.assertEquals("Cannot shutdown with active requests.", e.getMessage());
        }

        // May not have read all the data, in theory. Just enable auto-advance
        // and finish the request.
        callback.setAutoAdvance(true);
        callback.startNextRead(urlRequest);
        callback.blockForDone();
        callback.blockForCallbackToComplete();
        callback.shutdownExecutor();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testShutdownDuringInit() throws Exception {
        final ConditionVariable block = new ConditionVariable(false);

        // Post a task to main thread to block until shutdown is called to test
        // scenario when shutdown is called right after construction before
        // context is fully initialized on the main thread.
        Runnable blockingTask = new Runnable() {
            @Override
            public void run() {
                try {
                    block.block();
                } catch (Exception e) {
                    Assert.fail("Caught " + e.getMessage());
                }
            }
        };
        // Ensure that test is not running on the main thread.
        Assert.assertTrue(Looper.getMainLooper() != Looper.myLooper());
        new Handler(Looper.getMainLooper()).post(blockingTask);

        // Create new request context, but its initialization on the main thread
        // will be stuck behind blockingTask.
        final CronetUrlRequestContext cronetEngine =
                (CronetUrlRequestContext) new CronetEngine
                        .Builder(InstrumentationRegistry.getContext())
                        .build();
        // Unblock the main thread, so context gets initialized and shutdown on
        // it.
        block.open();
        // Shutdown will wait for init to complete on main thread.
        cronetEngine.shutdown();
        // Verify that context is shutdown.
        try {
            cronetEngine.getUrlRequestContextAdapter();
            Assert.fail("Should throw an exception.");
        } catch (Exception e) {
            Assert.assertEquals("Engine is shut down.", e.getMessage());
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testInitAndShutdownOnMainThread() throws Exception {
        final ConditionVariable block = new ConditionVariable(false);

        // Post a task to main thread to init and shutdown on the main thread.
        Runnable blockingTask = new Runnable() {
            @Override
            public void run() {
                // Create new request context, loading the library.
                final CronetUrlRequestContext cronetEngine =
                        (CronetUrlRequestContext) new CronetEngine
                                .Builder(InstrumentationRegistry.getContext())
                                .build();
                // Shutdown right after init.
                cronetEngine.shutdown();
                // Verify that context is shutdown.
                try {
                    cronetEngine.getUrlRequestContextAdapter();
                    Assert.fail("Should throw an exception.");
                } catch (Exception e) {
                    Assert.assertEquals("Engine is shut down.", e.getMessage());
                }
                block.open();
            }
        };
        new Handler(Looper.getMainLooper()).post(blockingTask);
        // Wait for shutdown to complete on main thread.
        block.block();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testMultipleShutdown() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        try {
            testFramework.mCronetEngine.shutdown();
            testFramework.mCronetEngine.shutdown();
            Assert.fail("Should throw an exception");
        } catch (Exception e) {
            Assert.assertEquals("Engine is shut down.", e.getMessage());
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    // TODO: Remove the annotation after fixing http://crbug.com/637972
    @OnlyRunNativeCronet
    public void testShutdownAfterError() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        ShutdownTestUrlRequestCallback callback =
                new ShutdownTestUrlRequestCallback(testFramework.mCronetEngine);
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                MOCK_CRONET_TEST_FAILED_URL, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertTrue(callback.mOnErrorCalled);
        callback.blockForCallbackToComplete();
        callback.shutdownExecutor();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testShutdownAfterCancel() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        // Block callback when response starts to verify that shutdown fails
        // if there are active requests.
        callback.setAutoAdvance(false);
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        UrlRequest urlRequest = urlRequestBuilder.build();
        urlRequest.start();
        try {
            testFramework.mCronetEngine.shutdown();
            Assert.fail("Should throw an exception");
        } catch (Exception e) {
            Assert.assertEquals("Cannot shutdown with active requests.", e.getMessage());
        }
        callback.waitForNextStep();
        Assert.assertEquals(ResponseStep.ON_RESPONSE_STARTED, callback.mResponseStep);
        urlRequest.cancel();
        testFramework.mCronetEngine.shutdown();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet // No netlogs for pure java impl
    public void testNetLog() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File file = File.createTempFile("cronet", "json", directory);
        CronetEngine cronetEngine = new CronetEngine.Builder(context).build();
        // Start NetLog immediately after the request context is created to make
        // sure that the call won't crash the app even when the native request
        // context is not fully initialized. See crbug.com/470196.
        cronetEngine.startNetLogToFile(file.getPath(), false);

        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        cronetEngine.stopNetLog();
        Assert.assertTrue(file.exists());
        Assert.assertTrue(file.length() != 0);
        Assert.assertFalse(hasBytesInNetLog(file));
        Assert.assertTrue(file.delete());
        Assert.assertTrue(!file.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet // No netlogs for pure java impl
    public void testBoundedFileNetLog() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File netLogDir = new File(directory, "NetLog");
        Assert.assertFalse(netLogDir.exists());
        Assert.assertTrue(netLogDir.mkdir());
        File eventFile = new File(netLogDir, "event_file_0.json");
        ExperimentalCronetEngine cronetEngine =
                new ExperimentalCronetEngine.Builder(context).build();
        // Start NetLog immediately after the request context is created to make
        // sure that the call won't crash the app even when the native request
        // context is not fully initialized. See crbug.com/470196.
        cronetEngine.startNetLogToDisk(netLogDir.getPath(), false, MAX_FILE_SIZE);

        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        cronetEngine.stopNetLog();
        Assert.assertTrue(eventFile.exists());
        Assert.assertTrue(eventFile.length() != 0);
        Assert.assertFalse(hasBytesInNetLog(eventFile));
        FileUtils.recursivelyDeleteFile(netLogDir);
        Assert.assertFalse(netLogDir.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet // No netlogs for pure java impl
    // Tests that if stopNetLog is not explicity called, CronetEngine.shutdown()
    // will take care of it. crbug.com/623701.
    public void testNoStopNetLog() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File file = File.createTempFile("cronet", "json", directory);
        CronetEngine cronetEngine = new CronetEngine.Builder(context).build();
        cronetEngine.startNetLogToFile(file.getPath(), false);

        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        // Shut down the engine without calling stopNetLog.
        cronetEngine.shutdown();
        Assert.assertTrue(file.exists());
        Assert.assertTrue(file.length() != 0);
        Assert.assertFalse(hasBytesInNetLog(file));
        Assert.assertTrue(file.delete());
        Assert.assertTrue(!file.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet // No netlogs for pure java impl
    // Tests that if stopNetLog is not explicity called, CronetEngine.shutdown()
    // will take care of it. crbug.com/623701.
    public void testNoStopBoundedFileNetLog() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File netLogDir = new File(directory, "NetLog");
        Assert.assertFalse(netLogDir.exists());
        Assert.assertTrue(netLogDir.mkdir());
        File eventFile = new File(netLogDir, "event_file_0.json");
        ExperimentalCronetEngine cronetEngine =
                new ExperimentalCronetEngine.Builder(context).build();
        cronetEngine.startNetLogToDisk(netLogDir.getPath(), false, MAX_FILE_SIZE);

        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        // Shut down the engine without calling stopNetLog.
        cronetEngine.shutdown();
        Assert.assertTrue(eventFile.exists());
        Assert.assertTrue(eventFile.length() != 0);

        FileUtils.recursivelyDeleteFile(netLogDir);
        Assert.assertFalse(netLogDir.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that NetLog contains events emitted by all live CronetEngines.
    public void testNetLogContainEventsFromAllLiveEngines() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File file1 = File.createTempFile("cronet1", "json", directory);
        File file2 = File.createTempFile("cronet2", "json", directory);
        CronetEngine cronetEngine1 = new CronetEngine.Builder(context).build();
        CronetEngine cronetEngine2 = new CronetEngine.Builder(context).build();

        cronetEngine1.startNetLogToFile(file1.getPath(), false);
        cronetEngine2.startNetLogToFile(file2.getPath(), false);

        // Warm CronetEngine and make sure both CronetUrlRequestContexts are
        // initialized before testing the logs.
        makeRequestAndCheckStatus(cronetEngine1, mUrl, 200);
        makeRequestAndCheckStatus(cronetEngine2, mUrl, 200);

        // Use cronetEngine1 to make a request to mUrl404.
        makeRequestAndCheckStatus(cronetEngine1, mUrl404, 404);

        // Use cronetEngine2 to make a request to mUrl500.
        makeRequestAndCheckStatus(cronetEngine2, mUrl500, 500);

        cronetEngine1.stopNetLog();
        cronetEngine2.stopNetLog();
        Assert.assertTrue(file1.exists());
        Assert.assertTrue(file2.exists());
        // Make sure both files contain the two requests made separately using
        // different engines.
        Assert.assertTrue(containsStringInNetLog(file1, mUrl404));
        Assert.assertTrue(containsStringInNetLog(file1, mUrl500));
        Assert.assertTrue(containsStringInNetLog(file2, mUrl404));
        Assert.assertTrue(containsStringInNetLog(file2, mUrl500));
        Assert.assertTrue(file1.delete());
        Assert.assertTrue(file2.delete());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that NetLog contains events emitted by all live CronetEngines.
    public void testBoundedFileNetLogContainEventsFromAllLiveEngines() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File netLogDir1 = new File(directory, "NetLog1");
        Assert.assertFalse(netLogDir1.exists());
        Assert.assertTrue(netLogDir1.mkdir());
        File netLogDir2 = new File(directory, "NetLog2");
        Assert.assertFalse(netLogDir2.exists());
        Assert.assertTrue(netLogDir2.mkdir());
        File eventFile1 = new File(netLogDir1, "event_file_0.json");
        File eventFile2 = new File(netLogDir2, "event_file_0.json");

        ExperimentalCronetEngine cronetEngine1 =
                new ExperimentalCronetEngine.Builder(context).build();
        ExperimentalCronetEngine cronetEngine2 =
                new ExperimentalCronetEngine.Builder(context).build();

        cronetEngine1.startNetLogToDisk(netLogDir1.getPath(), false, MAX_FILE_SIZE);
        cronetEngine2.startNetLogToDisk(netLogDir2.getPath(), false, MAX_FILE_SIZE);

        // Warm CronetEngine and make sure both CronetUrlRequestContexts are
        // initialized before testing the logs.
        makeRequestAndCheckStatus(cronetEngine1, mUrl, 200);
        makeRequestAndCheckStatus(cronetEngine2, mUrl, 200);

        // Use cronetEngine1 to make a request to mUrl404.
        makeRequestAndCheckStatus(cronetEngine1, mUrl404, 404);

        // Use cronetEngine2 to make a request to mUrl500.
        makeRequestAndCheckStatus(cronetEngine2, mUrl500, 500);

        cronetEngine1.stopNetLog();
        cronetEngine2.stopNetLog();

        Assert.assertTrue(eventFile1.exists());
        Assert.assertTrue(eventFile2.exists());
        Assert.assertTrue(eventFile1.length() != 0);
        Assert.assertTrue(eventFile2.length() != 0);

        // Make sure both files contain the two requests made separately using
        // different engines.
        Assert.assertTrue(containsStringInNetLog(eventFile1, mUrl404));
        Assert.assertTrue(containsStringInNetLog(eventFile1, mUrl500));
        Assert.assertTrue(containsStringInNetLog(eventFile2, mUrl404));
        Assert.assertTrue(containsStringInNetLog(eventFile2, mUrl500));

        FileUtils.recursivelyDeleteFile(netLogDir1);
        Assert.assertFalse(netLogDir1.exists());
        FileUtils.recursivelyDeleteFile(netLogDir2);
        Assert.assertFalse(netLogDir2.exists());
    }

    private CronetEngine createCronetEngineWithCache(int cacheType) {
        CronetEngine.Builder builder =
                new CronetEngine.Builder(InstrumentationRegistry.getContext());
        if (cacheType == CronetEngine.Builder.HTTP_CACHE_DISK) {
            builder.setStoragePath(
                    CronetTestRule.getTestStorage(InstrumentationRegistry.getContext()));
        }
        builder.enableHttpCache(cacheType, 100 * 1024);
        Assert.assertTrue(
                NativeTestServer.startNativeTestServer(InstrumentationRegistry.getContext()));
        return builder.build();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that if CronetEngine is shut down on the network thread, an appropriate exception
    // is thrown.
    public void testShutDownEngineOnNetworkThread() throws Exception {
        final CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_DISK);
        String url = NativeTestServer.getFileURL("/cacheable.txt");
        // Make a request to a cacheable resource.
        checkRequestCaching(cronetEngine, url, false);

        final AtomicReference<Throwable> thrown = new AtomicReference<>();
        // Shut down the server.
        NativeTestServer.shutdownNativeTestServer();
        class CancelUrlRequestCallback extends TestUrlRequestCallback {
            @Override
            public void onResponseStarted(UrlRequest request, UrlResponseInfo info) {
                super.onResponseStarted(request, info);
                request.cancel();
                // Shut down CronetEngine immediately after request is destroyed.
                try {
                    cronetEngine.shutdown();
                } catch (Exception e) {
                    thrown.set(e);
                }
            }

            @Override
            public void onSucceeded(UrlRequest request, UrlResponseInfo info) {
                // onSucceeded will not happen, because the request is canceled
                // after sending first read and the executor is single threaded.
                throw new RuntimeException("Unexpected");
            }

            @Override
            public void onFailed(UrlRequest request, UrlResponseInfo info, CronetException error) {
                throw new RuntimeException("Unexpected");
            }
        }
        Executor directExecutor = new Executor() {
            @Override
            public void execute(Runnable command) {
                command.run();
            }
        };
        CancelUrlRequestCallback callback = new CancelUrlRequestCallback();
        callback.setAllowDirectExecutor(true);
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(url, callback, directExecutor);
        urlRequestBuilder.allowDirectExecutor();
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertTrue(thrown.get() instanceof RuntimeException);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    // Tests that if CronetEngine is shut down when reading from disk cache,
    // there isn't a crash. See crbug.com/486120.
    public void testShutDownEngineWhenReadingFromDiskCache() throws Exception {
        final CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_DISK);
        String url = NativeTestServer.getFileURL("/cacheable.txt");
        // Make a request to a cacheable resource.
        checkRequestCaching(cronetEngine, url, false);

        // Shut down the server.
        NativeTestServer.shutdownNativeTestServer();
        class CancelUrlRequestCallback extends TestUrlRequestCallback {
            @Override
            public void onResponseStarted(UrlRequest request, UrlResponseInfo info) {
                super.onResponseStarted(request, info);
                request.cancel();
                // Shut down CronetEngine immediately after request is destroyed.
                cronetEngine.shutdown();
            }

            @Override
            public void onSucceeded(UrlRequest request, UrlResponseInfo info) {
                // onSucceeded will not happen, because the request is canceled
                // after sending first read and the executor is single threaded.
                throw new RuntimeException("Unexpected");
            }

            @Override
            public void onFailed(UrlRequest request, UrlResponseInfo info, CronetException error) {
                throw new RuntimeException("Unexpected");
            }
        }
        CancelUrlRequestCallback callback = new CancelUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
        Assert.assertTrue(callback.mResponseInfo.wasCached());
        Assert.assertTrue(callback.mOnCanceledCalled);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testNetLogAfterShutdown() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        testFramework.mCronetEngine.shutdown();

        File directory = new File(PathUtils.getDataDirectory());
        File file = File.createTempFile("cronet", "json", directory);
        try {
            testFramework.mCronetEngine.startNetLogToFile(file.getPath(), false);
            Assert.fail("Should throw an exception.");
        } catch (Exception e) {
            Assert.assertEquals("Engine is shut down.", e.getMessage());
        }
        Assert.assertFalse(hasBytesInNetLog(file));
        Assert.assertTrue(file.delete());
        Assert.assertTrue(!file.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testBoundedFileNetLogAfterShutdown() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        testFramework.mCronetEngine.shutdown();

        File directory = new File(PathUtils.getDataDirectory());
        File netLogDir = new File(directory, "NetLog");
        Assert.assertFalse(netLogDir.exists());
        Assert.assertTrue(netLogDir.mkdir());
        File constantsFile = new File(netLogDir, "constants.json");
        try {
            testFramework.mCronetEngine.startNetLogToDisk(
                    netLogDir.getPath(), false, MAX_FILE_SIZE);
            Assert.fail("Should throw an exception.");
        } catch (Exception e) {
            Assert.assertEquals("Engine is shut down.", e.getMessage());
        }
        Assert.assertFalse(constantsFile.exists());
        FileUtils.recursivelyDeleteFile(netLogDir);
        Assert.assertFalse(netLogDir.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testNetLogStartMultipleTimes() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        File directory = new File(PathUtils.getDataDirectory());
        File file = File.createTempFile("cronet", "json", directory);
        // Start NetLog multiple times.
        testFramework.mCronetEngine.startNetLogToFile(file.getPath(), false);
        testFramework.mCronetEngine.startNetLogToFile(file.getPath(), false);
        testFramework.mCronetEngine.startNetLogToFile(file.getPath(), false);
        testFramework.mCronetEngine.startNetLogToFile(file.getPath(), false);
        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        testFramework.mCronetEngine.stopNetLog();
        Assert.assertTrue(file.exists());
        Assert.assertTrue(file.length() != 0);
        Assert.assertFalse(hasBytesInNetLog(file));
        Assert.assertTrue(file.delete());
        Assert.assertTrue(!file.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testBoundedFileNetLogStartMultipleTimes() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        File directory = new File(PathUtils.getDataDirectory());
        File netLogDir = new File(directory, "NetLog");
        Assert.assertFalse(netLogDir.exists());
        Assert.assertTrue(netLogDir.mkdir());
        File eventFile = new File(netLogDir, "event_file_0.json");
        // Start NetLog multiple times. This should be equivalent to starting NetLog
        // once. Each subsequent start (without calling stopNetLog) should be a no-op.
        testFramework.mCronetEngine.startNetLogToDisk(netLogDir.getPath(), false, MAX_FILE_SIZE);
        testFramework.mCronetEngine.startNetLogToDisk(netLogDir.getPath(), false, MAX_FILE_SIZE);
        testFramework.mCronetEngine.startNetLogToDisk(netLogDir.getPath(), false, MAX_FILE_SIZE);
        testFramework.mCronetEngine.startNetLogToDisk(netLogDir.getPath(), false, MAX_FILE_SIZE);
        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        testFramework.mCronetEngine.stopNetLog();
        Assert.assertTrue(eventFile.exists());
        Assert.assertTrue(eventFile.length() != 0);
        Assert.assertFalse(hasBytesInNetLog(eventFile));
        FileUtils.recursivelyDeleteFile(netLogDir);
        Assert.assertFalse(netLogDir.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testNetLogStopMultipleTimes() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        File directory = new File(PathUtils.getDataDirectory());
        File file = File.createTempFile("cronet", "json", directory);
        testFramework.mCronetEngine.startNetLogToFile(file.getPath(), false);
        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        // Stop NetLog multiple times.
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        Assert.assertTrue(file.exists());
        Assert.assertTrue(file.length() != 0);
        Assert.assertFalse(hasBytesInNetLog(file));
        Assert.assertTrue(file.delete());
        Assert.assertTrue(!file.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testBoundedFileNetLogStopMultipleTimes() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();
        File directory = new File(PathUtils.getDataDirectory());
        File netLogDir = new File(directory, "NetLog");
        Assert.assertFalse(netLogDir.exists());
        Assert.assertTrue(netLogDir.mkdir());
        File eventFile = new File(netLogDir, "event_file_0.json");
        testFramework.mCronetEngine.startNetLogToDisk(netLogDir.getPath(), false, MAX_FILE_SIZE);
        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        // Stop NetLog multiple times. This should be equivalent to stopping NetLog once.
        // Each subsequent stop (without calling startNetLogToDisk first) should be a no-op.
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        testFramework.mCronetEngine.stopNetLog();
        Assert.assertTrue(eventFile.exists());
        Assert.assertTrue(eventFile.length() != 0);
        Assert.assertFalse(hasBytesInNetLog(eventFile));
        FileUtils.recursivelyDeleteFile(netLogDir);
        Assert.assertFalse(netLogDir.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testNetLogWithBytes() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File file = File.createTempFile("cronet", "json", directory);
        CronetEngine cronetEngine = new CronetEngine.Builder(context).build();
        // Start NetLog with logAll as true.
        cronetEngine.startNetLogToFile(file.getPath(), true);
        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        cronetEngine.stopNetLog();
        Assert.assertTrue(file.exists());
        Assert.assertTrue(file.length() != 0);
        Assert.assertTrue(hasBytesInNetLog(file));
        Assert.assertTrue(file.delete());
        Assert.assertTrue(!file.exists());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testBoundedFileNetLogWithBytes() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        File directory = new File(PathUtils.getDataDirectory());
        File netLogDir = new File(directory, "NetLog");
        Assert.assertFalse(netLogDir.exists());
        Assert.assertTrue(netLogDir.mkdir());
        File eventFile = new File(netLogDir, "event_file_0.json");
        ExperimentalCronetEngine cronetEngine =
                new ExperimentalCronetEngine.Builder(context).build();
        // Start NetLog with logAll as true.
        cronetEngine.startNetLogToDisk(netLogDir.getPath(), true, MAX_FILE_SIZE);
        // Start a request.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        cronetEngine.stopNetLog();

        Assert.assertTrue(eventFile.exists());
        Assert.assertTrue(eventFile.length() != 0);
        Assert.assertTrue(hasBytesInNetLog(eventFile));
        FileUtils.recursivelyDeleteFile(netLogDir);
        Assert.assertFalse(netLogDir.exists());
    }

    private boolean hasBytesInNetLog(File logFile) throws Exception {
        return containsStringInNetLog(logFile, "\"hex_encoded_bytes\"");
    }

    private boolean containsStringInNetLog(File logFile, String content) throws Exception {
        BufferedReader logReader = new BufferedReader(new FileReader(logFile));
        try {
            String logLine;
            while ((logLine = logReader.readLine()) != null) {
                if (logLine.contains(content)) {
                    return true;
                }
            }
            return false;
        } finally {
            logReader.close();
        }
    }

    /**
     * Helper method to make a request to {@code url}, wait for it to
     * complete, and check that the status code is the same as {@code expectedStatusCode}.
     */
    private void makeRequestAndCheckStatus(
            CronetEngine engine, String url, int expectedStatusCode) {
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest request =
                engine.newUrlRequestBuilder(url, callback, callback.getExecutor()).build();
        request.start();
        callback.blockForDone();
        Assert.assertEquals(expectedStatusCode, callback.mResponseInfo.getHttpStatusCode());
    }

    private void checkRequestCaching(CronetEngine engine, String url, boolean expectCached) {
        checkRequestCaching(engine, url, expectCached, false);
    }

    private void checkRequestCaching(
            CronetEngine engine, String url, boolean expectCached, boolean disableCache) {
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                engine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        if (disableCache) {
            urlRequestBuilder.disableCache();
        }
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertEquals(expectCached, callback.mResponseInfo.wasCached());
        Assert.assertEquals("this is a cacheable file\n", callback.mResponseAsString);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testEnableHttpCacheDisabled() throws Exception {
        CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_DISABLED);
        String url = NativeTestServer.getFileURL("/cacheable.txt");
        checkRequestCaching(cronetEngine, url, false);
        checkRequestCaching(cronetEngine, url, false);
        checkRequestCaching(cronetEngine, url, false);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testEnableHttpCacheInMemory() throws Exception {
        CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_IN_MEMORY);
        String url = NativeTestServer.getFileURL("/cacheable.txt");
        checkRequestCaching(cronetEngine, url, false);
        checkRequestCaching(cronetEngine, url, true);
        NativeTestServer.shutdownNativeTestServer();
        checkRequestCaching(cronetEngine, url, true);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testEnableHttpCacheDisk() throws Exception {
        CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_DISK);
        String url = NativeTestServer.getFileURL("/cacheable.txt");
        checkRequestCaching(cronetEngine, url, false);
        checkRequestCaching(cronetEngine, url, true);
        NativeTestServer.shutdownNativeTestServer();
        checkRequestCaching(cronetEngine, url, true);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testEnableHttpCacheDiskNoHttp() throws Exception {
        // TODO(pauljensen): This should be testing HTTP_CACHE_DISK_NO_HTTP.
        CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_DISABLED);
        String url = NativeTestServer.getFileURL("/cacheable.txt");
        checkRequestCaching(cronetEngine, url, false);
        checkRequestCaching(cronetEngine, url, false);
        checkRequestCaching(cronetEngine, url, false);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testDisableCache() throws Exception {
        CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_DISK);
        String url = NativeTestServer.getFileURL("/cacheable.txt");

        // When cache is disabled, making a request does not write to the cache.
        checkRequestCaching(cronetEngine, url, false, true /** disable cache */);
        checkRequestCaching(cronetEngine, url, false);

        // When cache is enabled, the second request is cached.
        checkRequestCaching(cronetEngine, url, false, true /** disable cache */);
        checkRequestCaching(cronetEngine, url, true);

        // Shut down the server, next request should have a cached response.
        NativeTestServer.shutdownNativeTestServer();
        checkRequestCaching(cronetEngine, url, true);

        // Cache is disabled after server is shut down, request should fail.
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        urlRequestBuilder.disableCache();
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertNotNull(callback.mError);
        CronetTestRule.assertContains("Exception in CronetUrlRequest: net::ERR_CONNECTION_REFUSED",
                callback.mError.getMessage());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testEnableHttpCacheDiskNewEngine() throws Exception {
        CronetEngine cronetEngine =
                createCronetEngineWithCache(CronetEngine.Builder.HTTP_CACHE_DISK);
        String url = NativeTestServer.getFileURL("/cacheable.txt");
        checkRequestCaching(cronetEngine, url, false);
        checkRequestCaching(cronetEngine, url, true);
        NativeTestServer.shutdownNativeTestServer();
        checkRequestCaching(cronetEngine, url, true);

        // Shutdown original context and create another that uses the same cache.
        cronetEngine.shutdown();
        cronetEngine = mTestRule
                               .enableDiskCache(new CronetEngine.Builder(
                                       InstrumentationRegistry.getContext()))
                               .build();
        checkRequestCaching(cronetEngine, url, true);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testInitEngineAndStartRequest() {
        // Immediately make a request after initializing the engine.
        CronetEngine cronetEngine =
                new CronetEngine.Builder(InstrumentationRegistry.getContext()).build();
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testEmptyGetCertVerifierData() {
        // Immediately make a request after initializing the engine.
        ExperimentalCronetEngine cronetEngine =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getContext()).build();
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder urlRequestBuilder =
                cronetEngine.newUrlRequestBuilder(mUrl, callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertEquals(200, callback.mResponseInfo.getHttpStatusCode());

        try {
            cronetEngine.getCertVerifierData(-1);
            Assert.fail("Should throw an exception");
        } catch (Exception e) {
            Assert.assertEquals("timeout must be a positive value", e.getMessage());
        }
        // Because mUrl is http, getCertVerifierData() will return empty data.
        String data = cronetEngine.getCertVerifierData(100);
        Assert.assertTrue(data.isEmpty());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testInitEngineStartTwoRequests() throws Exception {
        // Make two requests after initializing the context.
        CronetEngine cronetEngine =
                new CronetEngine.Builder(InstrumentationRegistry.getContext()).build();
        int[] statusCodes = {0, 0};
        String[] urls = {mUrl, mUrl404};
        for (int i = 0; i < 2; i++) {
            TestUrlRequestCallback callback = new TestUrlRequestCallback();
            UrlRequest.Builder urlRequestBuilder =
                    cronetEngine.newUrlRequestBuilder(urls[i], callback, callback.getExecutor());
            urlRequestBuilder.build().start();
            callback.blockForDone();
            statusCodes[i] = callback.mResponseInfo.getHttpStatusCode();
        }
        Assert.assertEquals(200, statusCodes[0]);
        Assert.assertEquals(404, statusCodes[1]);
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testInitTwoEnginesSimultaneously() throws Exception {
        // Threads will block on runBlocker to ensure simultaneous execution.
        ConditionVariable runBlocker = new ConditionVariable(false);
        RequestThread thread1 = new RequestThread(mUrl, runBlocker);
        RequestThread thread2 = new RequestThread(mUrl404, runBlocker);

        thread1.start();
        thread2.start();
        runBlocker.open();
        thread1.join();
        thread2.join();
        Assert.assertEquals(200, thread1.mCallback.mResponseInfo.getHttpStatusCode());
        Assert.assertEquals(404, thread2.mCallback.mResponseInfo.getHttpStatusCode());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testInitTwoEnginesInSequence() throws Exception {
        ConditionVariable runBlocker = new ConditionVariable(true);
        RequestThread thread1 = new RequestThread(mUrl, runBlocker);
        RequestThread thread2 = new RequestThread(mUrl404, runBlocker);

        thread1.start();
        thread1.join();
        thread2.start();
        thread2.join();
        Assert.assertEquals(200, thread1.mCallback.mResponseInfo.getHttpStatusCode());
        Assert.assertEquals(404, thread2.mCallback.mResponseInfo.getHttpStatusCode());
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testInitDifferentEngines() throws Exception {
        // Test that concurrently instantiating Cronet context's upon various
        // different versions of the same Android Context does not cause crashes
        // like crbug.com/453845
        CronetEngine firstEngine =
                new CronetEngine.Builder(InstrumentationRegistry.getContext()).build();
        CronetEngine secondEngine =
                new CronetEngine
                        .Builder(InstrumentationRegistry.getContext().getApplicationContext())
                        .build();
        CronetEngine thirdEngine =
                new CronetEngine.Builder(new ContextWrapper(InstrumentationRegistry.getContext()))
                        .build();
        firstEngine.shutdown();
        secondEngine.shutdown();
        thirdEngine.shutdown();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testGetGlobalMetricsDeltas() throws Exception {
        final CronetTestFramework testFramework = mTestRule.startCronetTestFramework();

        byte delta1[] = testFramework.mCronetEngine.getGlobalMetricsDeltas();

        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder builder = testFramework.mCronetEngine.newUrlRequestBuilder(
                mUrl, callback, callback.getExecutor());
        builder.build().start();
        callback.blockForDone();
        // Fetch deltas on a different thread the second time to make sure this is permitted.
        // See crbug.com/719448
        FutureTask<byte[]> task = new FutureTask<byte[]>(new Callable<byte[]>() {
            public byte[] call() {
                return testFramework.mCronetEngine.getGlobalMetricsDeltas();
            }
        });
        new Thread(task).start();
        byte delta2[] = task.get();
        Assert.assertTrue(delta2.length != 0);
        Assert.assertFalse(Arrays.equals(delta1, delta2));
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testCronetEngineBuilderConfig() throws Exception {
        // This is to prompt load of native library.
        mTestRule.startCronetTestFramework();
        // Verify CronetEngine.Builder config is passed down accurately to native code.
        ExperimentalCronetEngine.Builder builder =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getContext());
        builder.enableHttp2(false);
        builder.enableQuic(true);
        builder.enableSdch(true);
        builder.addQuicHint("example.com", 12, 34);
        builder.setCertVerifierData("test_cert_verifier_data");
        builder.enableHttpCache(HTTP_CACHE_IN_MEMORY, 54321);
        builder.setUserAgent("efgh");
        builder.setExperimentalOptions("ijkl");
        builder.setStoragePath(CronetTestRule.getTestStorage(InstrumentationRegistry.getContext()));
        builder.enablePublicKeyPinningBypassForLocalTrustAnchors(false);
        nativeVerifyUrlRequestContextConfig(
                CronetUrlRequestContext.createNativeUrlRequestContextConfig(
                        (CronetEngineBuilderImpl) builder.mBuilderDelegate),
                CronetTestRule.getTestStorage(InstrumentationRegistry.getContext()));
    }

    // Verifies that CronetEngine.Builder config from testCronetEngineBuilderConfig() is properly
    // translated to a native UrlRequestContextConfig.
    private static native void nativeVerifyUrlRequestContextConfig(long config, String storagePath);

    private static class TestBadLibraryLoader extends CronetEngine.Builder.LibraryLoader {
        private boolean mWasCalled = false;

        public void loadLibrary(String libName) {
            // Report that this method was called, but don't load the library
            mWasCalled = true;
        }

        boolean wasCalled() {
            return mWasCalled;
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testSkipLibraryLoading() throws Exception {
        CronetEngine.Builder builder =
                new CronetEngine.Builder(InstrumentationRegistry.getContext());
        TestBadLibraryLoader loader = new TestBadLibraryLoader();
        builder.setLibraryLoader(loader);
        try {
            // ensureInitialized() calls native code to check the version right after library load
            // and will error with the message below if library loading was skipped
            CronetLibraryLoader.ensureInitialized(
                    InstrumentationRegistry.getContext().getApplicationContext(),
                    (CronetEngineBuilderImpl) builder.mBuilderDelegate);
            Assert.fail("Native library should not be loaded");
        } catch (UnsatisfiedLinkError e) {
            Assert.assertTrue(loader.wasCalled());
        }
    }

    // Creates a CronetEngine on another thread and then one on the main thread.  This shouldn't
    // crash.
    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testThreadedStartup() throws Exception {
        final ConditionVariable otherThreadDone = new ConditionVariable();
        final ConditionVariable uiThreadDone = new ConditionVariable();
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            public void run() {
                final ExperimentalCronetEngine.Builder builder =
                        new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getContext());
                new Thread() {
                    public void run() {
                        CronetEngine cronetEngine = builder.build();
                        otherThreadDone.open();
                        cronetEngine.shutdown();
                    }
                }.start();
                otherThreadDone.block();
                builder.build().shutdown();
                uiThreadDone.open();
            }
        });
        Assert.assertTrue(uiThreadDone.block(1000));
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testHostResolverRules() throws Exception {
        String resolverTestHostname = "some-weird-hostname";
        URL testUrl = new URL(mUrl);
        ExperimentalCronetEngine.Builder cronetEngineBuilder =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getContext());
        JSONObject hostResolverRules = new JSONObject().put(
                "host_resolver_rules", "MAP " + resolverTestHostname + " " + testUrl.getHost());
        JSONObject experimentalOptions =
                new JSONObject().put("HostResolverRules", hostResolverRules);
        cronetEngineBuilder.setExperimentalOptions(experimentalOptions.toString());

        final CronetEngine cronetEngine = cronetEngineBuilder.build();
        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        URL requestUrl =
                new URL("http", resolverTestHostname, testUrl.getPort(), testUrl.getFile());
        UrlRequest.Builder urlRequestBuilder = cronetEngine.newUrlRequestBuilder(
                requestUrl.toString(), callback, callback.getExecutor());
        urlRequestBuilder.build().start();
        callback.blockForDone();
        Assert.assertEquals(200, callback.mResponseInfo.getHttpStatusCode());
    }

    /**
     * Runs {@code r} on {@code engine}'s network thread.
     */
    private static void postToNetworkThread(final CronetEngine engine, final Runnable r) {
        // Works by requesting an invalid URL which results in onFailed() being called, which is
        // done through a direct executor which causes onFailed to be run on the network thread.
        Executor directExecutor = new Executor() {
            @Override
            public void execute(Runnable runable) {
                runable.run();
            }
        };
        UrlRequest.Callback callback = new UrlRequest.Callback() {
            @Override
            public void onRedirectReceived(
                    UrlRequest request, UrlResponseInfo responseInfo, String newLocationUrl) {}
            @Override
            public void onResponseStarted(UrlRequest request, UrlResponseInfo responseInfo) {}
            @Override
            public void onReadCompleted(
                    UrlRequest request, UrlResponseInfo responseInfo, ByteBuffer byteBuffer) {}
            @Override
            public void onSucceeded(UrlRequest request, UrlResponseInfo responseInfo) {}

            @Override
            public void onFailed(
                    UrlRequest request, UrlResponseInfo responseInfo, CronetException error) {
                r.run();
            }
        };
        engine.newUrlRequestBuilder("", callback, directExecutor).build().start();
    }

    /**
     * @returns the thread priority of {@code engine}'s network thread.
     */
    private int getThreadPriority(CronetEngine engine) throws Exception {
        FutureTask<Integer> task = new FutureTask<Integer>(new Callable<Integer>() {
            public Integer call() {
                return Process.getThreadPriority(Process.myTid());
            }
        });
        postToNetworkThread(engine, task);
        return task.get();
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @RequiresMinApi(6) // setThreadPriority added in API 6: crrev.com/472449
    public void testCronetEngineThreadPriority() throws Exception {
        ExperimentalCronetEngine.Builder builder =
                new ExperimentalCronetEngine.Builder(InstrumentationRegistry.getContext());
        // Try out of bounds thread priorities.
        try {
            builder.setThreadPriority(-21);
            Assert.fail();
        } catch (IllegalArgumentException e) {
            Assert.assertEquals("Thread priority invalid", e.getMessage());
        }
        try {
            builder.setThreadPriority(20);
            Assert.fail();
        } catch (IllegalArgumentException e) {
            Assert.assertEquals("Thread priority invalid", e.getMessage());
        }
        // Test that valid thread priority range (-20..19) is working.
        for (int threadPriority = -20; threadPriority < 20; threadPriority++) {
            builder.setThreadPriority(threadPriority);
            CronetEngine engine = builder.build();
            Assert.assertEquals(threadPriority, getThreadPriority(engine));
            engine.shutdown();
        }
    }
}
