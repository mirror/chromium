package org.chromium.android_webview.pacservice;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;


import android.util.Log;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;

import java.util.concurrent.TimeUnit;

@JNINamespace("android_webview::pac_service")
class PacProcessor implements PacProcessorBackend {
    private static final String TAG = "PacProcessor";

    private final long mNativePacProcessor;
    private final HandlerThread mHandlerThread;
    private final Handler mHandler;
    private boolean mLoadedNative;
    private final Object mLock = new Object();

    private boolean mDone;
    private String mResult;

    private static final int MSG_START = 100;
    private static final int MSG_STOP = 101;
    private static final int MSG_SET = 102;
    private static final int MSG_RESOLVE = 103;

    private static final long RESOLVE_TIMEOUT_MS = TimeUnit.SECONDS.toMillis(3);

    public PacProcessor() {
        try {
            LibraryLoader.get(LibraryProcessType.PROCESS_WEBVIEW).loadNow();
            LibraryLoader.get(LibraryProcessType.PROCESS_WEBVIEW).ensureInitialized();

            mLoadedNative = true;
        } catch (ProcessInitException e) {
            Log.v(TAG, "could not load native library", e);
        }

        mNativePacProcessor = nativeInit();
        Log.v(TAG, "native is: " + mNativePacProcessor);
        mHandlerThread = new HandlerThread("PacProcessor");
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_START:
                        if (mLoadedNative) {
                            nativeStart(mNativePacProcessor);
                        } else {
                            Log.e(TAG, "Cannot start, native not loaded");
                        }
                        break;
                    case MSG_STOP:
                        if (mLoadedNative) {
                            nativeStop(mNativePacProcessor);
                        } else {
                            Log.e(TAG, "Cannot stop, native not loaded");
                        }
                        break;
                    case MSG_SET:
                        if (mLoadedNative) {
                            String content = (String) msg.obj;
                            nativeSetPacScript(mNativePacProcessor, content);
                        } else {
                            Log.e(TAG, "Cannot set, native not loaded");
                        }
                        break;
                    case MSG_RESOLVE:
                        if (mLoadedNative) {
                            String url = (String) msg.obj;
                            nativeResolve(mNativePacProcessor, url);
                        } else {
                            Log.e(TAG, "Cannot resolve, native not loaded");
                        }
                        break;
                    default:
                        throw new IllegalStateException();
                }
            }
        };
    }

    @Override
    public void start() {
        mHandler.sendEmptyMessage(MSG_START);
    }

    @Override
    public void stop() {
        mHandler.sendEmptyMessage(MSG_STOP);
    }

    @Override
    public String resolve(String host, String url) {
        long start = System.currentTimeMillis();
        synchronized (mLock) {
            mDone = false;
            mHandler.sendMessage(mHandler.obtainMessage(MSG_RESOLVE, url));

            while (!mDone) {
                try {
                    mLock.wait(RESOLVE_TIMEOUT_MS);
                } catch (InterruptedException e) {
                    Log.v(TAG, "Interrupted while waiting for resolve result", e);
                    return null;
                }
            }
        }
        Log.v(TAG, "resolve took " + (System.currentTimeMillis() - start) + "ms.");
        return mResult;
    }

    @Override
    public void setPacScript(String content) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SET, content));
    }

    // Only called by native; executes on mHandlerThread to wake up
    // the sleeping binder thread waiting for resolve result.
    @CalledByNative
    private void OnResolveResult(String result) {
        synchronized (mLock) {
            mDone = true;
            mResult = result;
            Log.v(TAG, "Result: " + result);
            mLock.notify();
        }
    }

    private native long nativeInit();
    private native void nativeStart(long nativePacProcessor);
    private native void nativeStop(long nativePacProcessor);
    private native void nativeSetPacScript(long nativePacProcessor, String script);
    private native void nativeResolve(long nativePacProcessor, String url);
}
