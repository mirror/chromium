// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.process_launcher;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.text.TextUtils;

import android.content.Intent;
import android.util.SparseArray;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.library_loader.Linker;
import org.chromium.base.process_launcher.ChildProcessCreationParams;
import org.chromium.base.process_launcher.FileDescriptorInfo;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import javax.annotation.concurrent.GuardedBy;

public class ChildProcessLauncher {
    private static final String TAG = "ChildProcLauncher";

    public interface Delegate {
        ChildProcessConnection getExistingConnection(
                ChildProcessConnection.StartCallback startCallback);

        void onBeforeConnectionAllocated(Bundle commonParameters);

        // Called once a connection has been allocated.
        // Implementations can modify the connectionBundle which will be passed to the service when
        // setting up the connections.
        void onConnectionBound(
                ChildProcessConnection connection, ChildConnectionAllocator connectionAllocator);

        void onBeforeConnectionSetup(ChildProcessConnection connection, Bundle connectionBundle);

        void onConnectionEstablished(ChildProcessConnection connection);

        // Called when a connection has been disconnected. Only invoked if onConnectionEstablished
        // was called, meaning the connection was already established.
        void onConnectionLost(ChildProcessConnection connection);
    }

    // Represents an invalid process handle; same as base/process/process.h kNullProcessHandle.
    private static final int NULL_PROCESS_HANDLE = 0;

    // Map from PID to ChildService connection.
    private static Map<Integer, ChildProcessLauncher> sLauncherByPid = new HashMap<>();

    // The handle for the thread we were created on and on which all methods should be called.
    private final Handler mLauncherHandler = new Handler();

    private final Delegate mDelegate;

    // Whether the connection has a strong binding (which also means it is not managed by the
    // BindingManager).
    private final boolean mUseStrongBinding;

    private final String[] mCommandLine;

    private final FileDescriptorInfo[] mFilesToBeMapped;

    // The allocator used to create the connection.
    private final ChildConnectionAllocator mConnectionAllocator;

    // The IBinder provided to the created service.
    private final IBinder mIBinderCallback;

    // The actual service connection. Set once we have connected to the service.
    private ChildProcessConnection mConnection;

    public static ChildProcessLauncher getByPid(int pid) {
        return sLauncherByPid.get(pid);
    }

    @SuppressFBWarnings("EI_EXPOSE_REP2")
    public ChildProcessLauncher(Delegate delegate, String[] commandLine,
            FileDescriptorInfo[] filesToBeMapped, ChildConnectionAllocator connectionAllocator,
            boolean useStrongBinding, IBinder binderCallback) {
        mCommandLine = commandLine;
        mConnectionAllocator = connectionAllocator;
        mDelegate = delegate;
        mFilesToBeMapped = filesToBeMapped;
        mIBinderCallback = binderCallback;
        mUseStrongBinding = useStrongBinding;
    }

    public void start(final boolean setupConnection, final boolean queueIfNoFreeConnection) {
        assert isRunningOnLauncherThread();
        assert mConnection == null;
        try {
            TraceEvent.begin("ChildProcessLauncher.start");

            ChildProcessConnection.StartCallback startCallback =
                    new ChildProcessConnection.StartCallback() {
                        @Override
                        public void onChildStarted() {}

                        @Override
                        public void onChildStartFailed() {
                            assert isRunningOnLauncherThread();
                            Log.e(TAG, "ChildProcessConnection.start failed, trying again");
                            mLauncherHandler.post(new Runnable() {
                                @Override
                                public void run() {
                                    // The child process may already be bound to another client
                                    // (this can happen if multi-process WebView is used in more
                                    // than one process), so try starting the process again.
                                    // This connection that failed to start has not been freed,
                                    // so a new bound connection will be allocated.
                                    mConnection = null;
                                    start(setupConnection, queueIfNoFreeConnection);
                                }
                            });
                        }
                    };
            allocateAndSetupConnection(startCallback, setupConnection, queueIfNoFreeConnection);
        } finally {
            TraceEvent.end("ChildProcessLauncher.start");
        }
    }

    public ChildProcessConnection getConnection() {
        return mConnection;
    }

    public ChildConnectionAllocator getConnectionAllocator() {
        return mConnectionAllocator;
    }

    private ChildProcessConnection allocateBoundConnection(Context context, Bundle serviceBundle,
            ChildProcessConnection.StartCallback startCallback,
            final ChildProcessConnection.DeathCallback deathCallback) {
        assert isRunningOnLauncherThread();

        ChildProcessConnection.DeathCallback deathCallbackWrapper =
                new ChildProcessConnection.DeathCallback() {
                    @Override
                    public void onChildProcessDied(ChildProcessConnection connection) {
                        assert isRunningOnLauncherThread();
                        if (connection.getPid() != 0) {
                            stop();
                        } else {
                            freeConnection();
                        }
                        // Forward the call to the provided callback if any. The spare connection
                        // uses that for clean-up.
                        if (deathCallback != null) {
                            deathCallback.onChildProcessDied(connection);
                        }
                    }
                };

        ChildProcessConnection connection =
                mConnectionAllocator.allocate(context, serviceBundle, deathCallbackWrapper);
        if (connection != null) {
            connection.start(mUseStrongBinding, startCallback);
        }
        return connection;
    }

    private boolean allocateAndSetupConnection(
            final ChildProcessConnection.StartCallback startCallback, final boolean setupConnection,
            final boolean queueIfNoFreeConnection) {
        mConnection = mDelegate.getExistingConnection(startCallback);
        if (mConnection == null) {
            final Context context = ContextUtils.getApplicationContext();
            Bundle serviceBundle = new Bundle();
            mDelegate.onBeforeConnectionAllocated(serviceBundle);

            mConnection = allocateBoundConnection(
                    context, serviceBundle, startCallback, null /* deathCallback */);
            if (mConnection == null) {
                if (!queueIfNoFreeConnection) {
                    Log.d(TAG, "Failed to allocate a child connection (no queuing).");
                    return false;
                }
                // No connection is available at this time. Add a listener so when one becomes
                // available we can create the service.
                mConnectionAllocator.addListener(new ChildConnectionAllocator.Listener() {
                    @Override
                    public void onConnectionFreed(
                            ChildConnectionAllocator allocator, ChildProcessConnection connection) {
                        assert allocator == mConnectionAllocator;
                        if (!allocator.isFreeConnectionAvailable()) return;
                        allocator.removeListener(this);
                        allocateAndSetupConnection(
                                startCallback, setupConnection, queueIfNoFreeConnection);
                    }
                });
                return false;
            }
        }

        assert mConnection != null;
        mDelegate.onConnectionBound(mConnection, mConnectionAllocator);

        if (setupConnection) {
            Bundle connectionBundle = createConnectionBundle();
            mDelegate.onBeforeConnectionSetup(mConnection, connectionBundle);
            setupConnection(connectionBundle);
        }
        return true;
    }

    private void setupConnection(Bundle connectionBundle) {
        ChildProcessConnection.ConnectionCallback connectionCallback =
                new ChildProcessConnection.ConnectionCallback() {
                    @Override
                    public void onConnected(ChildProcessConnection connection) {
                        assert mConnection == connection;
                        onChildProcessStarted();
                    }
                };
        mConnection.setupConnection(connectionBundle, getIBinderCallback(), connectionCallback);
    }

    private static final long FREE_CONNECTION_DELAY_MILLIS = 1;

    private void freeConnection() {
        assert isRunningOnLauncherThread();

        // Freeing a service should be delayed. This is so that we avoid immediately reusing the
        // freed service (see http://crbug.com/164069): the framework might keep a service process
        // alive when it's been unbound for a short time. If a new connection to the same service
        // is bound at that point, the process is reused and bad things happen (mostly static
        // variables are set when we don't expect them to).
        mLauncherHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mConnectionAllocator.free(mConnection);
                mConnection = null;
            }
        }, FREE_CONNECTION_DELAY_MILLIS);
    }

    public void onChildProcessStarted() {
        assert isRunningOnLauncherThread();

        int pid = mConnection.getPid();
        Log.d(TAG, "on connect callback, pid=%d", pid);

        mDelegate.onConnectionEstablished(mConnection);

        sLauncherByPid.put(pid, this);

        // Proactively close the FDs rather than waiting for the GC to do it.
        try {
            for (FileDescriptorInfo fileInfo : mFilesToBeMapped) {
                fileInfo.fd.close();
            }
        } catch (IOException ioe) {
            Log.w(TAG, "Failed to close FD.", ioe);
        }
    }

    public int getPid() {
        assert isRunningOnLauncherThread();
        return mConnection == null ? NULL_PROCESS_HANDLE : mConnection.getPid();
    }

    public IBinder getIBinderCallback() {
        return mIBinderCallback;
    }

    private boolean isRunningOnLauncherThread() {
        return mLauncherHandler.getLooper() == Looper.myLooper();
    }

    private Bundle createConnectionBundle() {
        Bundle bundle = new Bundle();
        bundle.putStringArray(ChildProcessConstants.EXTRA_COMMAND_LINE, mCommandLine);
        bundle.putParcelableArray(ChildProcessConstants.EXTRA_FILES, mFilesToBeMapped);
        return bundle;
    }

    public void stop() {
        assert isRunningOnLauncherThread();
        Log.d(TAG, "stopping child connection: pid=%d", mConnection.getPid());
        mDelegate.onConnectionLost(mConnection);
        mConnection.stop();
        freeConnection();
    }

    @VisibleForTesting
    public static boolean crashProcessForTesting(int pid) {
        if (sLauncherByPid.get(pid) == null) return false;

        ChildProcessConnection connection = sLauncherByPid.get(pid).mConnection;
        if (connection == null) return false;

        try {
            connection.crashServiceForTesting();
            return true;
        } catch (RemoteException ex) {
            return false;
        }
    }
}
