// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.devtools;

import org.chromium.base.ObserverList;
import org.chromium.base.annotations.CalledByNative;

/** Interface to the native content::DevToolsAgentHost for a given Tab's content::WebContents.
 *
 * See chrome/browser/android/devtools/devtools_agent_host_android.h for the native side of this
 * class.
 */
public class DevToolsAgentHostImpl implements DevToolsAgentHost {
    /**
     * Callback for nativeSendCommand. The native code generator has to see all types to generate
     * the appropriate bindings.
     */
    public static class ResultCallbackWrapper {
        private ResultCallback mCallback;

        public ResultCallbackWrapper(ResultCallback callback) {
            mCallback = callback;
        }

        public void onResult(String result, String error) {
            mCallback.onDevToolsCommandResult(result, error);
        }
    }

    private long mNative;
    private final ObserverList<EventListener> mListeners;

    public DevToolsAgentHostImpl(long devtoolsAgentHostNativePtr) {
        mNative = nativeInit(devtoolsAgentHostNativePtr);
        mListeners = new ObserverList<EventListener>();
    }

    @Override
    public void sendCommand(String method, String params, ResultCallback callback) {
        if (mNative == 0) {
            callback.onDevToolsCommandResult(null, "DevToolsAgentHost is closed");
        } else {
            nativeSendCommand(mNative, method, params, new ResultCallbackWrapper(callback));
        }
    }

    @Override
    public void addEventListener(EventListener listener) {
        mListeners.addObserver(listener);
    }

    @Override
    public void removeEventListener(EventListener listener) {
        mListeners.removeObserver(listener);
    }

    @Override
    public void close() {
        if (mNative != 0) {
            nativeDestroy(mNative);
            mNative = 0;
            mListeners.clear();
        }
    }

    @CalledByNative
    private static void onCommandResult(
            ResultCallbackWrapper callback, String result, String error) {
        if (result != null) {
            callback.onResult(result, null);
        } else if (error == null) {
            callback.onResult(null, "Missing result!?");
        } else {
            callback.onResult(null, error);
        }
    }

    @CalledByNative
    private void onEvent(String method, String params) {
        for (EventListener listener : mListeners) {
            listener.onDevToolsEvent(method, params);
        }
    }

    private native long nativeInit(long devtoolsAgentHostNativePtr);
    private native void nativeDestroy(long nativeDevToolsAgentHostAndroid);
    private native void nativeSendCommand(long nativeDevToolsAgentHostAndroid, String method,
            String params, ResultCallbackWrapper callback);
}
