// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.ComponentName;
import android.content.Context;
import android.os.Bundle;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.anyBoolean;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.process_launcher.ChildProcessCreationParams;
import org.chromium.base.test.util.Feature;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/** Unit tests for the SpareChildConnection class. */
@Config(manifest = Config.NONE)
@RunWith(LocalRobolectricTestRunner.class)
public class SpareChildConnectionTest {
    @Mock
    private ChildProcessConnection.StartCallback mStartCallback;

    static class TestConnectionFactory implements ChildConnectionAllocator.ConnectionFactory {
        private ChildProcessConnection mConnection;
        private ChildProcessConnection.DeathCallback mDeathCallback;
        private ArgumentCaptor<ChildProcessConnection.StartCallback> mStartCallbackCaptor;

        @Override
        public ChildProcessConnection createConnection(Context context, ComponentName serviceName,
                boolean bindAsExternalService, Bundle serviceBundle,
                ChildProcessCreationParams creationParams,
                ChildProcessConnection.DeathCallback deathCallback) {
            mConnection = mock(ChildProcessConnection.class);
            mStartCallbackCaptor =
                    ArgumentCaptor.forClass(ChildProcessConnection.StartCallback.class);
            doNothing().when(mConnection).start(anyBoolean(), mStartCallbackCaptor.capture());
            mDeathCallback = deathCallback;
            return mConnection;
        }

        public void simulateConnectionBindingSuccessfully() {
            mStartCallbackCaptor.getValue().onChildStarted();
        }

        public void simulateConnectionFailingToBind() {
            mStartCallbackCaptor.getValue().onChildStartFailed();
        }

        public void simulateConnectionDeath() {
            mDeathCallback.onChildProcessDied(mConnection);
        }
    }

    private final TestConnectionFactory mTestConnectionfactory = new TestConnectionFactory();

    private final ChildConnectionAllocator mConnectionAllocator =
            ChildConnectionAllocator.createForTest("org.chromium.content.test" /* packageName */,
                    "SpareConnTest" /* serviceClassName */, 10 /* serviceCount */,
                    false /* bindAsExternalService */);

    private final ChildConnectionAllocator mWrongConnectionAllocator =
            ChildConnectionAllocator.createForTest("org.chromium.content.test" /* packageName */,
                    "SpareConnTestBad" /* serviceClassName */, 10 /* serviceCount */,
                    false /* bindAsExternalService */);

    // For some reason creating ChildProcessCreationParams from a static context makes the launcher
    // unhappy. (some Dalvik native library is not found when initializing a SparseArray)
    private final ChildProcessCreationParams mCreationParams = new ChildProcessCreationParams(
            "org.chromium.test,.spare_connection", true /* isExternalService */,
            0 /* libraryProcessType */, true /* bindToCallerCheck */);

    private SpareChildConnection mSpareConnection;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        // The tests run on only one thread. Pretend that is the launcher thread so LauncherThread
        // asserts are not triggered.
        LauncherThread.setCurrentThreadAsLauncherThread();

        mConnectionAllocator.setConnectionFactoryForTesting(mTestConnectionfactory);

        mSpareConnection = new SpareChildConnection(null /* context */, mConnectionAllocator,
                false /* useStrongBinding */, new Bundle() /* serviceBundle */, mCreationParams);
    }

    @After
    public void tearDown() {
        LauncherThread.setLauncherThreadAsLauncherThread();
    }

    /** Test creation and retrieval of connection. */
    @Test
    @Feature({"ProcessManagement"})
    public void testCreateAndGet() {
        // Tests retrieving the connection with the wrong parameters.
        ChildProcessConnection connection = mSpareConnection.getConnection(
                mWrongConnectionAllocator, mCreationParams, mStartCallback);
        assertNull(connection);

        connection = mSpareConnection.getConnection(
                mConnectionAllocator, null /* creationParams */, mStartCallback);
        assertNull(connection);

        // And with the right one.
        connection = mSpareConnection.getConnection(
                mConnectionAllocator, mCreationParams, mStartCallback);
        assertNotNull(connection);

        // The connection has been used, subsequent calls should return null.
        connection = mSpareConnection.getConnection(
                mConnectionAllocator, mCreationParams, mStartCallback);
        assertNull(connection);
    }

    @Test
    @Feature({"ProcessManagement"})
    public void testCallbackNotCalledWhenNoConnection() {
        mTestConnectionfactory.simulateConnectionBindingSuccessfully();

        // Retrieve the wrong connection, no callback should be fired.
        ChildProcessConnection connection = mSpareConnection.getConnection(
                mWrongConnectionAllocator, mCreationParams, mStartCallback);
        assertNull(connection);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        verify(mStartCallback, times(0)).onChildStarted();
        verify(mStartCallback, times(0)).onChildStartFailed();
    }

    @Test
    @Feature({"ProcessManagement"})
    public void testCallbackCalledConnectionReady() {
        mTestConnectionfactory.simulateConnectionBindingSuccessfully();

        // Now retrieve the connection, the callback should be invoked.
        ChildProcessConnection connection = mSpareConnection.getConnection(
                mConnectionAllocator, mCreationParams, mStartCallback);
        assertNotNull(connection);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        verify(mStartCallback, times(1)).onChildStarted();
        verify(mStartCallback, times(0)).onChildStartFailed();
    }

    @Test
    @Feature({"ProcessManagement"})
    public void testCallbackCalledConnectionNotReady() {
        // Retrieve the connection before it's bound.
        ChildProcessConnection connection = mSpareConnection.getConnection(
                mConnectionAllocator, mCreationParams, mStartCallback);
        assertNotNull(connection);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        // No callbacks are called.
        verify(mStartCallback, times(0)).onChildStarted();
        verify(mStartCallback, times(0)).onChildStartFailed();

        // Simulate the connection getting bound, it should trigger the callback.
        mTestConnectionfactory.simulateConnectionBindingSuccessfully();
        verify(mStartCallback, times(1)).onChildStarted();
        verify(mStartCallback, times(0)).onChildStartFailed();
    }

    @Test
    @Feature({"ProcessManagement"})
    public void testUnretrievedConnectionFailsToBind() {
        mTestConnectionfactory.simulateConnectionFailingToBind();

        // We should not have a spare connection.
        ChildProcessConnection connection = mSpareConnection.getConnection(
                mConnectionAllocator, mCreationParams, mStartCallback);
        assertNull(connection);
    }

    @Test
    @Feature({"ProcessManagement"})
    public void testRetrievedConnectionFailsToBind() {
        // Retrieve the spare connection before it's bound.
        ChildProcessConnection connection = mSpareConnection.getConnection(
                mConnectionAllocator, mCreationParams, mStartCallback);
        assertNotNull(connection);

        mTestConnectionfactory.simulateConnectionFailingToBind();

        // We should get a failure callback.
        verify(mStartCallback, times(0)).onChildStarted();
        verify(mStartCallback, times(1)).onChildStartFailed();
    }

    @Test
    @Feature({"ProcessManagement"})
    public void testConnectionFreeing() {
        // Simulate the connection dying.
        mTestConnectionfactory.simulateConnectionDeath();

        // Connection should be gone.
        ChildProcessConnection connection = mSpareConnection.getConnection(
                mConnectionAllocator, mCreationParams, mStartCallback);
        assertNull(connection);
    }
}
