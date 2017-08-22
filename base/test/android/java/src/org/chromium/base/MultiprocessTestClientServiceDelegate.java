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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

/** Implementation of the ChildProcessServiceDelegate used for the Multiprocess tests. */
public class MultiprocessTestClientServiceDelegate implements ChildProcessServiceDelegate {
    private static final String TAG = "MPTestCSDelegate";

    private ITestCallback mTestCallback;

    private final ITestController.Stub mTestController = new ITestController.Stub() {
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
    public void onConnectionSetup(Bundle connectionBundle, List<IBinder> callbacks) {
        mTestCallback = ITestCallback.Stub.asInterface(callbacks.get(0));
        String initClassName = connectionBundle.getString(
                MultiprocessTestClientLauncher.DELEGATE_INIT_CLASS_EXTRA);
        if (initClassName != null) {
            // We use reflection as a way to customize the service without having to declare a new
            // service for each customization.
            try {
                Class<?> initClass = Class.forName(initClassName);
                Method initMethod =
                        initClass.getMethod("initializeMultiprocessTestService", List.class);
                initMethod.invoke(null /* object */, callbacks.subList(1, callbacks.size()));
            } catch (ClassNotFoundException cnfe) {
                Log.e(TAG, "Failed to find initialization delegate class " + initClassName);
            } catch (NoSuchMethodException nsme) {
                Log.e(TAG,
                        "Failed to perform delegate initialization, no initialize method in "
                                + " class " + initClassName);
            } catch (IllegalAccessException iae) {
                Log.e(TAG, "Failed to access initialization delegate class " + initClassName, iae);
            } catch (InvocationTargetException ite) {
                Log.e(TAG, "Failed to invoke initialization delegate class " + initClassName, ite);
            }
        }
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
        }
    }
}
