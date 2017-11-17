// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

/**
 * Shell class for services provided by WebAPK to Chrome. Extracts code with implementation of
 * services from .dex file in Chrome APK.
 */
public class WebApkServiceFactory extends Service {
    private IBinder mIBinder;

    @Override
    public IBinder onBind(Intent intent) {
        if (mIBinder == null) {
            mIBinder = new WebApkServiceImplWrapper(getApplicationContext());
        }
        return mIBinder;
    }
}
