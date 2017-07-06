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

import org.chromium.base.Log;
import org.chromium.base.process_launcher.ChildProcessService;
import org.chromium.base.process_launcher.ChildProcessServiceDelegate;

/**
 * Child service started by ChildProcessLauncherTest.
 */
public class TestChildProcessService extends ChildProcessService {
    private static final String TAG = "TestProcessService";

    private static class TestChildProcessServiceDelegate implements ChildProcessServiceDelegate {
        private boolean mServiceCreated;
        private Bundle mServiceBundle;
        private IChildProcessTest mIChildProcessTest;

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
            mIChildProcessTest =
                    callback != null ? IChildProcessTest.Stub.asInterface(callback) : null;
            if (mIChildProcessTest != null) {
                try {
                    mIChildProcessTest.connectionSetup(
                            mServiceCreated, mServiceBundle, connectionBundle);
                } catch (RemoteException re) {
                    Log.e(TAG, "Failed to call IChildProcessTest.connectionSetup.", re);
                }
            }
        }

        @Override
        public void onDestroy() {}

        @Override
        public boolean loadNativeLibrary(Context hostContext) {
            return true;
        }

        @Override
        public SparseArray<String> getFileDescriptorsIdsToKeys() {
            return null;
        }

        @Override
        public void onBeforeMain() {}

        @Override
        public void runMain() {}
    };

    public TestChildProcessService() {
        super(new TestChildProcessServiceDelegate());
    }
}
