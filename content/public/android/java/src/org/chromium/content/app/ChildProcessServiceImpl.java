// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.app;

import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.Process;
import android.os.RemoteException;

import org.chromium.base.BaseSwitches;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.annotations.UsedByReflection;
import org.chromium.base.process_launcher.FileDescriptorInfo;
import org.chromium.base.process_launcher.ICallbackInt;
import org.chromium.base.process_launcher.IChildProcessService;
import org.chromium.content.browser.ChildProcessConstants;

import java.util.concurrent.Semaphore;

import javax.annotation.concurrent.GuardedBy;

/**
 * This class implements all of the functionality for {@link ChildProcessService} which owns an
 * object of {@link ChildProcessServiceImpl}.
 * It makes possible that WebAPK's ChildProcessService owns a ChildProcessServiceImpl object
 * and uses the same functionalities to create renderer process for WebAPKs when
 * "--enable-improved-a2hs" flag is turned on.
 */
@JNINamespace("content")
@MainDex
@UsedByReflection("WebApkSandboxedProcessService")
public class ChildProcessServiceImpl {
    private static final String MAIN_THREAD_NAME = "ChildProcessMain";
    private static final String TAG = "ChildProcessService";

    // Only for a check that create is only called once.
    private static boolean sCreateCalled;

    private final ChildProcessServiceDelegate mDelegate;

    private final Object mConnectionInitializedLock = new Object();
    private final Object mLibraryInitializedLock = new Object();
    private final Object mBinderLock = new Object();

    // Whether we should enforce that bindToCaller() is called before setupConnection().
    @GuardedBy("mBinderLock")
    private boolean mBindToCallerCheck;

    // PID of the client of this service, set in bindToCaller(), if mBindToCallerCheck is true.
    @GuardedBy("mBinderLock")
    private int mBoundCallingPid;

    // This is the native "Main" thread for the renderer / utility process.
    private Thread mMainThread;

    // Parameters received via IPC, only accessed while holding the mMainThread monitor.
    private String[] mCommandLineParams;

    // File descriptors that should be registered natively.
    private FileDescriptorInfo[] mFdInfos;

    @GuardedBy("mLibraryInitializedLock")
    private boolean mLibraryInitialized;

    // Called once the service is bound and all service related member variables have been set.
    private boolean mServiceBound;

    // Called once the connection has been setup (meaning the client has called setupConnection) and
    // all connection related member variables have been set.
    @GuardedBy("mConnectionInitializedLock")
    private boolean mConnectionInitialized;
    /**
     * If >= 0 enables "validation of caller of {@link mBinder}'s methods". A RemoteException
     * is thrown when an application with a uid other than {@link mAuthorizedCallerUid} calls
     * {@link mBinder}'s methods.
     */
    private int mAuthorizedCallerUid;

    private final Semaphore mActivitySemaphore = new Semaphore(1);

    @UsedByReflection("WebApkSandboxedProcessService")
    public ChildProcessServiceImpl(ChildProcessServiceDelegate delegate) {
        KillChildUncaughtExceptionHandler.maybeInstallHandler();
        mDelegate = delegate;
    }

    // Binder object used by clients for this service.
    private final IChildProcessService.Stub mBinder = new IChildProcessService.Stub() {
        // NOTE: Implement any IChildProcessService methods here.
        @Override
        public boolean bindToCaller() {
            synchronized (mBinderLock) {
                assert mBindToCallerCheck;
                int callingPid = Binder.getCallingPid();
                if (mBoundCallingPid == 0) {
                    mBoundCallingPid = callingPid;
                } else if (mBoundCallingPid != callingPid) {
                    Log.e(TAG, "Service is already bound by pid %d, cannot bind for pid %d",
                            mBoundCallingPid, callingPid);
                    return false;
                }
                return true;
            }
        }

        @Override
        public void setupConnection(Bundle args, ICallbackInt pidCallback, IBinder callback)
                throws RemoteException {
            synchronized (mBinderLock) {
                if (mBindToCallerCheck && mBoundCallingPid == 0) {
                    Log.e(TAG, "Service has not been bound with bindToCaller()");
                    pidCallback.call(-1);
                    return;
                }
            }

            pidCallback.call(Process.myPid());
            processConnectionBundle(args);
            mDelegate.onConnectionSetup(args, callback);
            synchronized (mConnectionInitializedLock) {
                mConnectionInitialized = true;
                mConnectionInitializedLock.notifyAll();
            }
        }

        @Override
        public void crashIntentionallyForTesting() {
            Process.killProcess(Process.myPid());
        }

        @Override
        public boolean onTransact(int arg0, Parcel arg1, Parcel arg2, int arg3)
                throws RemoteException {
            if (mAuthorizedCallerUid >= 0) {
                int callingUid = Binder.getCallingUid();
                if (callingUid != mAuthorizedCallerUid) {
                    throw new RemoteException("Unauthorized caller " + callingUid
                            + "does not match expected host=" + mAuthorizedCallerUid);
                }
            }
            return super.onTransact(arg0, arg1, arg2, arg3);
        }
    };

    // The ClassLoader for the host context.
    private ClassLoader mHostClassLoader;

    /**
     * Loads Chrome's native libraries and initializes a ChildProcessServiceImpl.
     * @param context The application context.
     * @param hostContext The host context the library should be loaded with (i.e. Chrome).
     */
    @SuppressFBWarnings("ST_WRITE_TO_STATIC_FROM_INSTANCE_METHOD") // For sCreateCalled check.
    @UsedByReflection("WebApkSandboxedProcessService")
    public void create(final Context context, final Context hostContext) {
        mHostClassLoader = hostContext.getClassLoader();
        Log.i(TAG, "Creating new ChildProcessService pid=%d", Process.myPid());
        if (sCreateCalled) {
            throw new RuntimeException("Illegal child process reuse.");
        }
        sCreateCalled = true;

        mDelegate.onServiceCreated();

        // Initialize the context for the application that owns this ChildProcessServiceImpl object.
        ContextUtils.initApplicationContext(context);

        mMainThread = new Thread(new Runnable() {
            @Override
            @SuppressFBWarnings("DM_EXIT")
            public void run()  {
                try {
                    // Wait for the connection to be initialized so the command line, fds and others
                    // are available.
                    synchronized (mConnectionInitializedLock) {
                        while (!mConnectionInitialized) {
                            mConnectionInitializedLock.wait();
                        }
                    }
                    assert mCommandLineParams != null;
                    assert mFdInfos != null;
                    CommandLine.init(mCommandLineParams);

                    if (CommandLine.getInstance().hasSwitch(
                            BaseSwitches.RENDERER_WAIT_FOR_JAVA_DEBUGGER)) {
                        android.os.Debug.waitForDebugger();
                    }

                    if (!mDelegate.loadNativeLibrary(hostContext)) {
                        System.exit(-1);
                    }
                    synchronized (mLibraryInitializedLock) {
                        mLibraryInitialized = true;
                        mLibraryInitializedLock.notifyAll();
                    }

                    int[] fileIds = new int[mFdInfos.length];
                    int[] fds = new int[mFdInfos.length];
                    long[] regionOffsets = new long[mFdInfos.length];
                    long[] regionSizes = new long[mFdInfos.length];
                    for (int i = 0; i < mFdInfos.length; i++) {
                        FileDescriptorInfo fdInfo = mFdInfos[i];
                        fileIds[i] = fdInfo.id;
                        fds[i] = fdInfo.fd.detachFd();
                        regionOffsets[i] = fdInfo.offset;
                        regionSizes[i] = fdInfo.size;
                    }
                    nativeRegisterFileDescriptors(fileIds, fds, regionOffsets, regionSizes);

                    mDelegate.onBeforeMain();
                    if (mActivitySemaphore.tryAcquire()) {
                        mDelegate.runMain();
                        nativeExitChildProcess();
                    }
                } catch (InterruptedException e) {
                    Log.w(TAG, "%s startup failed: %s", MAIN_THREAD_NAME, e);
                }
            }
        }, MAIN_THREAD_NAME);
        mMainThread.start();
    }

    @SuppressFBWarnings("DM_EXIT")
    public void destroy() {
        Log.i(TAG, "Destroying ChildProcessService pid=%d", Process.myPid());
        if (mActivitySemaphore.tryAcquire()) {
            // TODO(crbug.com/457406): This is a bit hacky, but there is no known better solution
            // as this service will get reused (at least if not sandboxed).
            // In fact, we might really want to always exit() from onDestroy(), not just from
            // the early return here.
            System.exit(0);
            return;
        }
        synchronized (mLibraryInitializedLock) {
            try {
                while (!mLibraryInitialized) {
                    // Avoid a potential race in calling through to native code before the library
                    // has loaded.
                    mLibraryInitializedLock.wait();
                }
            } catch (InterruptedException e) {
                // Ignore
            }
        }
        mDelegate.onDestroy();
    }

    /*
     * Returns the communication channel to service. Note that if multiple clients were to connect
     * should only get one call to this method. So there is no need to synchronize on any member
     * variables initialized here and accessed from the binder returned here, as the binder can only
     * be used after this method returns.
     * @param intent The intent that was used to bind to the service.
     * @param authorizedCallerUid If >= 0, enables "validation of service caller". A RemoteException
     *        is thrown when an application with a uid other than
     *        {@link authorizedCallerUid} calls the service's methods.
     */
    @UsedByReflection("WebApkSandboxedProcessService")
    public IBinder bind(Intent intent, int authorizedCallerUid) {
        assert !mServiceBound;
        mAuthorizedCallerUid = authorizedCallerUid;
        synchronized (mBinderLock) {
            mBindToCallerCheck =
                    intent.getBooleanExtra(ChildProcessConstants.EXTRA_BIND_TO_CALLER, false);
        }
        mServiceBound = true;
        mDelegate.onServiceBound(intent);
        return mBinder;
    }

    private void processConnectionBundle(Bundle bundle) {
        // Required to unparcel FileDescriptorInfo.
        bundle.setClassLoader(mHostClassLoader);

        assert mCommandLineParams == null;
        mCommandLineParams = bundle.getStringArray(ChildProcessConstants.EXTRA_COMMAND_LINE);
        Parcelable[] fdInfosAsParcelable =
                bundle.getParcelableArray(ChildProcessConstants.EXTRA_FILES);
        if (fdInfosAsParcelable != null) {
            // For why this arraycopy is necessary:
            // http://stackoverflow.com/questions/8745893/i-dont-get-why-this-classcastexception-occurs
            mFdInfos = new FileDescriptorInfo[fdInfosAsParcelable.length];
            System.arraycopy(fdInfosAsParcelable, 0, mFdInfos, 0, fdInfosAsParcelable.length);
        }
    }

    /**
     * Helper for registering FileDescriptorInfo objects with GlobalFileDescriptors or
     * FileDescriptorStore.
     * This includes the IPC channel, the crash dump signals and resource related
     * files.
     */
    private static native void nativeRegisterFileDescriptors(
            int[] id, int[] fd, long[] offset, long[] size);

    /**
     * Force the child process to exit.
     */
    private static native void nativeExitChildProcess();
}
