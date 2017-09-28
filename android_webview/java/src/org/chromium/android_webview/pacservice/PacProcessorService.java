// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.pacservice;

import android.app.Service;
import android.content.Intent;
import com.android.net.IProxyService;
import android.os.IBinder;
import android.util.Log;

import org.chromium.base.ContextUtils;

public class PacProcessorService extends Service {

    private final static String LOGTAG = "WVPacProcessor";

    private final Object mLock = new Object();

    private PacProcessorBackend mBackend;

    private final IProxyService.Stub mBinder = new IProxyService.Stub() {

        @Override
        public void setPacFile(String script) {
            Log.i(LOGTAG, "set");
            synchronized (mLock) {
                mBackend.setPacScript(script);
            }
        }

        @Override
        public String resolvePacFile(String host, String url) {
            Log.i(LOGTAG, "resolve");
            synchronized (mLock) {
                return mBackend.resolve(host, url);
            }
        }

        @Override
        public void startPacSystem() {
            Log.i(LOGTAG, "start");
            if (mBackend == null) {
                ContextUtils.initApplicationContext(PacProcessorService.this.getApplicationContext());
                mBackend = new PacProcessor();
            }

            synchronized (mLock) {
                  mBackend.start();
            }
        }

        @Override
        public void stopPacSystem() {
            Log.i(LOGTAG, "stop");
            synchronized (mLock) {
                mBackend.stop();
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(LOGTAG, "Bound");
        return mBinder;
    }
}
