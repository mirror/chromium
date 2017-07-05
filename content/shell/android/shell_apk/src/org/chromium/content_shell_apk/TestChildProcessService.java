// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell_apk;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.SparseArray;

import org.chromium.base.CommandLine;
import org.chromium.base.Log;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.process_launcher.ChildProcessService;
import org.chromium.base.process_launcher.ChildProcessServiceDelegate;

/**
 * Child service started by ChildProcessLauncherTest.
 */
public class TestChildProcessService extends ChildProcessService {
    private static final String TAG = "TestProcessService";

    private static final long MAIN_BLOCKING_DURATION_MS = 5000;

    private static class TestChildProcessServiceDelegate implements ChildProcessServiceDelegate {
        private final Object mIChildProcessTestLock = new Object();
        // Note that we are not guarding mIChildProcessTest with mIChildProcessTestLock as the lock
        // is only used to wait for mIChildProcessTest to be set in loadNativeLibrary (accessing it
        // when loading the library only happens in this test). For the other delegate callbacks,
        // mIChildProcessTest is guaranteed to be set.
        private IChildProcessTest mIChildProcessTest;

        private boolean mServiceCreated;
        private Bundle mServiceBundle;
        private String[] mCommandLine;

        @Override
        public void onServiceCreated() {
            mServiceCreated = true;
        }

        @Override
        public void onServiceBound(Intent intent) {
            mServiceBundle = intent.getExtras();
        }

        @Override
        public void onConnectionSetup(Bundle connectionBundle, IBinder callback) {
            assert callback != null;
            synchronized (mIChildProcessTestLock) {
                mIChildProcessTest = IChildProcessTest.Stub.asInterface(callback);
                mIChildProcessTestLock.notifyAll();
            }
            try {
                mIChildProcessTest.onConnectionSetup(
                        mServiceCreated, mServiceBundle, connectionBundle);
            } catch (RemoteException re) {
                Log.e(TAG, "Failed to call IChildProcessTest.onConnectionSetup.", re);
            }
        }

        @Override
        public void onDestroy() {
            try {
                mIChildProcessTest.onDestroy();
            } catch (RemoteException re) {
                Log.e(TAG, "Failed to call IChildProcessTest.onDestroy.", re);
            }
        }

        @Override
        public boolean loadNativeLibrary(Context hostContext) {
            // Store the command line before loading the library to avoid an assert in CommandLine.
            mCommandLine = CommandLine.getJavaSwitchesOrNull();

            LibraryLoader libraryLoader = null;
            boolean isLoaded = false;
            try {
                libraryLoader = LibraryLoader.get(LibraryProcessType.PROCESS_CHILD);
                libraryLoader.loadNow();
                libraryLoader.ensureInitialized();
                isLoaded = true;
            } catch (ProcessInitException e) {
                Log.e(TAG, "Failed to load native library.", e);
            }

            // Loading the library happen on the main thread and onConnectionSetup is called from
            // the client. Wait for onConnectionSetup so mIChildProcessTest is set.
            synchronized (mIChildProcessTestLock) {
                while (mIChildProcessTest == null) {
                    try {
                        mIChildProcessTestLock.wait();
                    } catch (InterruptedException e) {
                        // Ignore.
                    }
                }
            }
            try {
                mIChildProcessTest.onLoadNativeLibrary(isLoaded);
            } catch (RemoteException re) {
                Log.e(TAG, "Failed to call IChildProcessTest.onLoadNativeLibrary.", re);
            }
            return true;
        }

        @Override
        public SparseArray<String> getFileDescriptorsIdsToKeys() {
            return null;
        }

        @Override
        public void onBeforeMain() {
            try {
                mIChildProcessTest.onBeforeMain(mCommandLine);
            } catch (RemoteException re) {
                Log.e(TAG, "Failed to call IChildProcessTest.onBeforeMain.", re);
            }
        }

        @Override
        public void runMain() {
            try {
                mIChildProcessTest.onRunMain();
            } catch (RemoteException re) {
                Log.e(TAG, "Failed to call IChildProcessTest.onRunMain.", re);
            }
        }
    };

    public TestChildProcessService() {
        super(new TestChildProcessServiceDelegate());
    }
}
