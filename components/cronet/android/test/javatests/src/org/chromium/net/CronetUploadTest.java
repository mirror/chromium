// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

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
import org.chromium.net.CronetTestRule.OnlyRunNativeCronet;
import org.chromium.net.impl.CronetUploadDataStream;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Tests that directly drive {@code CronetUploadDataStream} and
 * {@code UploadDataProvider} to simulate different ordering of reset, init,
 * read, and rewind calls.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CronetUploadTest {
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    private TestDrivenDataProvider mDataProvider;
    private CronetUploadDataStream mUploadDataStream;
    private TestUploadDataStreamHandler mHandler;

    @Before
    @SuppressWarnings("PrimitiveArrayPassedToVarargsMethod")
    public void setUp() throws Exception {
        mTestRule.startCronetTestFramework();
        ExecutorService executor = Executors.newSingleThreadExecutor();
        List<byte[]> reads = Arrays.asList("hello".getBytes());
        mDataProvider = new TestDrivenDataProvider(executor, reads);
        mUploadDataStream = new CronetUploadDataStream(mDataProvider, executor);
        mHandler = new TestUploadDataStreamHandler(InstrumentationRegistry.getTargetContext(),
                mUploadDataStream.createUploadDataStreamForTesting());
    }

    @After
    public void tearDown() throws Exception {
        // Destroy handler's native objects.
        mHandler.destroyNativeObjects();
    }

    /**
     * Tests that after some data is read, init triggers a rewind, and that
     * before the rewind completes, init blocks.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testInitTriggersRewindAndInitBeforeRewindCompletes() throws Exception {
        // Init completes synchronously and read succeeds.
        Assert.assertTrue(mHandler.init());
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();
        mDataProvider.onReadSucceeded(mUploadDataStream);
        mHandler.waitForReadComplete();
        mDataProvider.assertReadNotPending();
        Assert.assertEquals(0, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("hello", mHandler.getData());

        // Reset and then init, which should trigger a rewind.
        mHandler.reset();
        Assert.assertEquals("", mHandler.getData());
        Assert.assertFalse(mHandler.init());
        mDataProvider.waitForRewindRequest();
        mHandler.checkInitCallbackNotInvoked();

        // Before rewind completes, reset and init should block.
        mHandler.reset();
        Assert.assertFalse(mHandler.init());

        // Signal rewind completes, and wait for init to complete.
        mHandler.checkInitCallbackNotInvoked();
        mDataProvider.onRewindSucceeded(mUploadDataStream);
        mHandler.waitForInitComplete();
        mDataProvider.assertRewindNotPending();

        // Read should complete successfully since init has completed.
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();
        mDataProvider.onReadSucceeded(mUploadDataStream);
        mHandler.waitForReadComplete();
        mDataProvider.assertReadNotPending();
        Assert.assertEquals(1, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(2, mDataProvider.getNumReadCalls());
        Assert.assertEquals("hello", mHandler.getData());
    }

    /**
     * Tests that after some data is read, init triggers a rewind, and that
     * after the rewind completes, init does not block.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testInitTriggersRewindAndInitAfterRewindCompletes() throws Exception {
        // Init completes synchronously and read succeeds.
        Assert.assertTrue(mHandler.init());
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();
        mDataProvider.onReadSucceeded(mUploadDataStream);
        mHandler.waitForReadComplete();
        mDataProvider.assertReadNotPending();
        Assert.assertEquals(0, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("hello", mHandler.getData());

        // Reset and then init, which should trigger a rewind.
        mHandler.reset();
        Assert.assertEquals("", mHandler.getData());
        Assert.assertFalse(mHandler.init());
        mDataProvider.waitForRewindRequest();
        mHandler.checkInitCallbackNotInvoked();

        // Signal rewind completes, and wait for init to complete.
        mDataProvider.onRewindSucceeded(mUploadDataStream);
        mHandler.waitForInitComplete();
        mDataProvider.assertRewindNotPending();

        // Reset and init should not block, since rewind has completed.
        mHandler.reset();
        Assert.assertTrue(mHandler.init());

        // Read should complete successfully since init has completed.
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();
        mDataProvider.onReadSucceeded(mUploadDataStream);
        mHandler.waitForReadComplete();
        mDataProvider.assertReadNotPending();
        Assert.assertEquals(1, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(2, mDataProvider.getNumReadCalls());
        Assert.assertEquals("hello", mHandler.getData());
    }

    /**
     * Tests that if init before read completes, a rewind is triggered when
     * read completes.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testReadCompleteTriggerRewind() throws Exception {
        // Reset and init before read completes.
        Assert.assertTrue(mHandler.init());
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();
        mHandler.reset();
        // Init should return asynchronously, since there is a pending read.
        Assert.assertFalse(mHandler.init());
        mDataProvider.assertRewindNotPending();
        mHandler.checkInitCallbackNotInvoked();
        Assert.assertEquals(0, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("", mHandler.getData());

        // Read completes should trigger a rewind.
        mDataProvider.onReadSucceeded(mUploadDataStream);
        mDataProvider.waitForRewindRequest();
        mHandler.checkInitCallbackNotInvoked();
        mDataProvider.onRewindSucceeded(mUploadDataStream);
        mHandler.waitForInitComplete();
        mDataProvider.assertRewindNotPending();
        Assert.assertEquals(1, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("", mHandler.getData());
    }

    /**
     * Tests that when init again after rewind completes, no additional rewind
     * is triggered. This test is the same as testReadCompleteTriggerRewind
     * except that this test invokes reset and init again in the end.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testReadCompleteTriggerRewindOnlyOneRewind() throws Exception {
        testReadCompleteTriggerRewind();
        // Reset and Init again, no rewind should happen.
        mHandler.reset();
        Assert.assertTrue(mHandler.init());
        mDataProvider.assertRewindNotPending();
        Assert.assertEquals(1, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("", mHandler.getData());
    }

    /**
     * Tests that if reset before read completes, no rewind is triggered, and
     * that a following init triggers rewind.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testResetBeforeReadCompleteAndInitTriggerRewind() throws Exception {
        // Reset before read completes. Rewind is not triggered.
        Assert.assertTrue(mHandler.init());
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();
        mHandler.reset();
        mDataProvider.onReadSucceeded(mUploadDataStream);
        mDataProvider.assertRewindNotPending();
        Assert.assertEquals(0, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("", mHandler.getData());

        // Init should trigger a rewind.
        Assert.assertFalse(mHandler.init());
        mDataProvider.waitForRewindRequest();
        mHandler.checkInitCallbackNotInvoked();
        mDataProvider.onRewindSucceeded(mUploadDataStream);
        mHandler.waitForInitComplete();
        mDataProvider.assertRewindNotPending();
        Assert.assertEquals(1, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("", mHandler.getData());
    }

    /**
     * Tests that there is no crash when native CronetUploadDataStream is
     * destroyed while read is pending. The test is racy since the read could
     * complete either before or after the Java CronetUploadDataStream's
     * onDestroyUploadDataStream() method is invoked. However, the test should
     * pass either way, though we are interested in the latter case.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testDestroyNativeStreamBeforeReadComplete() throws Exception {
        // Start a read and wait for it to be pending.
        Assert.assertTrue(mHandler.init());
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();

        // Destroy the C++ TestUploadDataStreamHandler. The handler will then
        // destroy the C++ CronetUploadDataStream it owns on the network thread.
        // That will result in calling the Java CronetUploadDataSteam's
        // onUploadDataStreamDestroyed() method on its executor thread, which
        // will then destroy the CronetUploadDataStreamAdapter.
        mHandler.destroyNativeObjects();

        // Make the read complete should not encounter a crash.
        mDataProvider.onReadSucceeded(mUploadDataStream);

        Assert.assertEquals(0, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
    }

    /**
     * Tests that there is no crash when native CronetUploadDataStream is
     * destroyed while rewind is pending. The test is racy since rewind could
     * complete either before or after the Java CronetUploadDataStream's
     * onDestroyUploadDataStream() method is invoked. However, the test should
     * pass either way, though we are interested in the latter case.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testDestroyNativeStreamBeforeRewindComplete() throws Exception {
        // Start a read and wait for it to complete.
        Assert.assertTrue(mHandler.init());
        mHandler.read();
        mDataProvider.waitForReadRequest();
        mHandler.checkReadCallbackNotInvoked();
        mDataProvider.onReadSucceeded(mUploadDataStream);
        mHandler.waitForReadComplete();
        mDataProvider.assertReadNotPending();
        Assert.assertEquals(0, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
        Assert.assertEquals("hello", mHandler.getData());

        // Reset and then init, which should trigger a rewind.
        mHandler.reset();
        Assert.assertEquals("", mHandler.getData());
        Assert.assertFalse(mHandler.init());
        mDataProvider.waitForRewindRequest();
        mHandler.checkInitCallbackNotInvoked();

        // Destroy the C++ TestUploadDataStreamHandler. The handler will then
        // destroy the C++ CronetUploadDataStream it owns on the network thread.
        // That will result in calling the Java CronetUploadDataSteam's
        // onUploadDataStreamDestroyed() method on its executor thread, which
        // will then destroy the CronetUploadDataStreamAdapter.
        mHandler.destroyNativeObjects();

        // Signal rewind completes, and wait for init to complete.
        mDataProvider.onRewindSucceeded(mUploadDataStream);

        Assert.assertEquals(1, mDataProvider.getNumRewindCalls());
        Assert.assertEquals(1, mDataProvider.getNumReadCalls());
    }
}
