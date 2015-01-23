// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.SparseArray;
import android.webkit.ValueCallback;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

/**
 * Provides the Message Channel functionality for Android Webview. Specifically
 * manages the message ports that are associated with a message channel and
 * handles posting/receiving messages to/from them.
 * See https://html.spec.whatwg.org/multipage/comms.html#messagechannel for
 * further information on message channels.
 *
 * The message ports have unique IDs. In Android webview implementation,
 * the message ports are only known by their IDs at the native side.
 * At the java side, the embedder deals with MessagePort objects. The mapping
 * from an ID to an object is in AwMessagePortService. AwMessagePortService
 * keeps a strong ref to MessagePort objects until they are closed.
 *
 * Ownership: The Java AwMessagePortService is owned by Java AwBrowserContext.
 * The native AwMessagePortService is owned by native AwBrowserContext. The
 * native peer maintains a weak ref to the java object and deregisters itself
 * before being deleted.
 *
 * All methods are called on UI thread except as noted.
 */
@JNINamespace("android_webview")
public class AwMessagePortService {

    private static final String TAG = "AwMessagePortService";

    private static final int POST_MESSAGE = 1;

    // TODO(sgurun) implement transferring ports from JS to Java using message channels.
    private static class PostMessage {
        public MessagePort port;
        public String message;

        public PostMessage(MessagePort port, String message) {
            this.port = port;
            this.message = message;
        }
    }

    // The messages from JS to Java are posted to a message port on a background thread.
    // We do this to make any potential synchronization between Java and JS
    // easier for user programs.
    private static class MessageHandlerThread extends Thread {
        private static class MessageHandler extends Handler {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == POST_MESSAGE) {
                    PostMessage m = (PostMessage) msg.obj;
                    m.port.onMessage(m.message);
                    return;
                }
                throw new IllegalStateException("undefined message");
            }
        }

        private MessageHandler mHandler;

        public Handler getHandler() {
            return mHandler;
        }

        public void run() {
            Looper.prepare();
            mHandler = new MessageHandler();
            Looper.loop();
        }
    }

    // A thread safe storage for Message Ports.
    private static class MessagePortStorage {
        private SparseArray<MessagePort> mMessagePorts = new SparseArray<MessagePort>();
        private Object mLock = new Object();

        public void put(int portId, MessagePort m) {
            synchronized (mLock) {
                mMessagePorts.put(portId, m);
            }
        }
        public MessagePort get(int portId) {
            synchronized (mLock) {
                return mMessagePorts.get(portId);
            }
        }
    }

    private long mNativeMessagePortService;
    private MessagePortStorage mPortStorage = new MessagePortStorage();
    private MessageHandlerThread mMessageHandlerThread = new MessageHandlerThread();

    AwMessagePortService() {
        mNativeMessagePortService = nativeInitAwMessagePortService();
        mMessageHandlerThread.start();
    }

    private MessagePort addPort(int portId) {
        if (mPortStorage.get(portId) != null) {
            throw new IllegalStateException("Port already exists");
        }
        MessagePort m = new MessagePort(portId);
        mPortStorage.put(portId, m);
        return m;
    }

    @CalledByNative
    private void onMessageChannelCreated(int portId1, int portId2,
            ValueCallback<MessageChannel> callback) {
        callback.onReceiveValue(new MessageChannel(addPort(portId1), addPort(portId2)));
    }

    // Called on IO thread.
    @CalledByNative
    private void onPostMessage(int portId, String message, int[] ports) {
        PostMessage m = new PostMessage(mPortStorage.get(portId), message);
        Handler handler = mMessageHandlerThread.getHandler();
        Message msg = handler.obtainMessage(POST_MESSAGE, m);
        handler.sendMessage(msg);
    }

    @CalledByNative
    private void unregisterNativeAwMessagePortService() {
        mNativeMessagePortService = 0;
    }

    private native long nativeInitAwMessagePortService();
}
