// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.SparseArray;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.process_launcher.ChildProcessServiceDelegate;
import org.chromium.native_test.MainRunner;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import javax.annotation.concurrent.GuardedBy;

/** Implmentation of the ChildProcessServiceDelegate used for the Multiprocess tests. */
public class MultiprocessTestClientServiceDelegate implements ChildProcessServiceDelegate {
    private static final String TAG = "MPTestCSDelegate";

    private final ReentrantLock mResultLock = new ReentrantLock();
    private final Condition mResultCondition = mResultLock.newCondition();
    private final Condition mClientReceivedResultCondition = mResultLock.newCondition();

    @GuardedBy("mResultLock")
    private MainReturnCodeResult mResult;

    private ITestCallback mTestCallback;

    private boolean mWaitingForMainToReturn;

    // Whether we got an ACK from the client that it received the main's result.
    private boolean mClientReceivedResult;

    private final ITestController.Stub mTestController = new ITestController.Stub() {
        @SuppressFBWarnings("RCN_REDUNDANT_NULLCHECK_OF_NULL_VALUE")
        @Override
        public MainReturnCodeResult waitForMainToReturn(long timeoutMs) {
            mWaitingForMainToReturn = true;
            boolean timedOut = false;
            mResultLock.lock();
            try {
                while (mResult == null && !timedOut) {
                    try {
                        timedOut = !mResultCondition.await(timeoutMs, TimeUnit.MILLISECONDS);
                    } catch (InterruptedException ie) {
                        continue;
                    }
                }
            } finally {
                mResultLock.unlock();
            }
            if (timedOut) {
                Log.e(TAG, "Failed to wait for main return value.");
                return new MainReturnCodeResult(0, true /* timed-out */);
            }
            return mResult;
        }

        @Override
        public void mainResultReceived() {
            assert mWaitingForMainToReturn;
            assert !mClientReceivedResult;
            mResultLock.lock();
            try {
                mClientReceivedResult = true;
                mClientReceivedResultCondition.signal();
            } finally {
                mResultLock.unlock();
            }
        }

        @SuppressFBWarnings("DM_EXIT")
        @Override
        public boolean forceStopSynchronous(int exitCode) {
            System.exit(exitCode);
            return true;
        }

        @SuppressFBWarnings("DM_EXIT")
        @Override
        public void forceStop(int exitCode) {
            System.exit(exitCode);
        }
    };

    @Override
    public void onServiceCreated() {
        PathUtils.setPrivateDataDirectorySuffix("chrome_multiprocess_test_client_service");
    }

    @Override
    public void onServiceBound(Intent intent) {}

    @Override
    public void onConnectionSetup(Bundle connectionBundle, IBinder callback) {
        mTestCallback = ITestCallback.Stub.asInterface(callback);
    }

    @Override
    public void onDestroy() {}

    @Override
    public boolean loadNativeLibrary(Context hostContext) {
        try {
            LibraryLoader.get(LibraryProcessType.PROCESS_CHILD).loadNow();
            return true;
        } catch (ProcessInitException pie) {
            Log.e(TAG, "Unable to load native libraries.", pie);
            return false;
        }
    }

    @Override
    public SparseArray<String> getFileDescriptorsIdsToKeys() {
        return null;
    }

    @Override
    public void onBeforeMain() {
        try {
            mTestCallback.childConnected(mTestController);
        } catch (RemoteException re) {
            Log.e(TAG, "Failed to notify parent process of connection.");
        }
    }

    @Override
    public void runMain() {
        int result = MainRunner.runMain(CommandLine.getJavaSwitchesOrNull());
        // Set the result and unblock waitForMainToReturn().
        mResultLock.lock();
        try {
            mResult = new MainReturnCodeResult(result, false /* timed-out */);
            mResultCondition.signal();

            // When waitForMainToReturn() was called, we'll wait for an ACK from the client before
            // returning from this method, as when  we retun the service will be stopped natively
            // and the background thread handling the waitForMainToReturn might not have sent back
            // the result yet.
            while (mWaitingForMainToReturn && !mClientReceivedResult) {
                try {
                    mClientReceivedResultCondition.await();
                } catch (InterruptedException ie) {
                    continue;
                }
            }
        } finally {
            mResultLock.unlock();
        }
    }
}
