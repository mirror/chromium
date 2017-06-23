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

/** Implmentation of the ChildProcessServiceDelegate used for the Multiprocess tests. */
public class MultiprocessTestClientServiceDelegate implements ChildProcessServiceDelegate {
    private static final String TAG = "MPTestCSDelegate";

    private final Object mClientReceivedMainResultLock = new Object();

    private ITestCallback mTestCallback;

    // Whether we got an ACK from the client that it received the main's result.
    private boolean mClientReceivedMainResult;

    private final ITestController.Stub mTestController = new ITestController.Stub() {
        @Override
        public void mainResultReceived() {
            synchronized (mClientReceivedMainResultLock) {
                mClientReceivedMainResult = true;
                mClientReceivedMainResultLock.notifyAll();
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
        try {
            mTestCallback.mainReturned(result);
        } catch (RemoteException re) {
            Log.e(TAG, "Failed to notify parent process of main returning.");
            return;
        }

        // Wait for an ACK from the client before returning from this method, as when  we return the
        // service will be stopped natively and the background thread handling the binder call above
        // might not have been done yet.
        synchronized (mClientReceivedMainResultLock) {
            while (!mClientReceivedMainResult) {
                try {
                    mClientReceivedMainResultLock.wait();
                } catch (InterruptedException ie) {
                    Log.e(TAG, "Interrupted while waiting for main result ack.");
                }
            }
        }
    }
}
