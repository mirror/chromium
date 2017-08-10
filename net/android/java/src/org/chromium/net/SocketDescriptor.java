// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;

import org.chromium.base.BaseChromiumApplication;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.webapk.lib.runtime_library.IWebApkApi;

import java.io.IOException;

/**
 * Utility functions for verifying X.509 certificates.
 */
@JNINamespace("net")
public class SocketDescriptor {
//    @CalledByNative
    private static int createSocket(int family, int type, int protocol) {
        try {
            BaseChromiumApplication app = (BaseChromiumApplication) ContextUtils.getApplicationContext();
            return IWebApkApi.Stub.asInterface((IBinder) app.thing).createSocket(family, type, protocol).detachFd();
        } catch (RemoteException e) {
            return -1;

        }
    }

    @CalledByNative
    private static void tagSocket(int fd) {
        try {
            BaseChromiumApplication app = (BaseChromiumApplication) ContextUtils.getApplicationContext();
            Log.e("Yaron", "tag fd " + fd + " thing=" + app.thing);
            IWebApkApi.Stub.asInterface((IBinder) app.thing).tagSocket(ParcelFileDescriptor.fromFd(fd));
        } catch (RemoteException e) {
            e.printStackTrace();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

