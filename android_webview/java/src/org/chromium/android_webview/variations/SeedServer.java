// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;

/**
 * SeedServer is a bound service that shares the Variations seed with all the WebViews
 * on the system. A WebView will bind and call getSeed, passing a file descriptor to which the
 * service should write the seed.
 */
public class SeedServer extends Service {
    private SeedHolder mSeedHolder;

    private final ISeedServer.Stub mBinder = new ISeedServer.Stub() {
        @Override
        public void getSeed(ParcelFileDescriptor dest) {
            mSeedHolder.writeSeed(dest);
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        ContextUtils.initApplicationContext(this.getApplicationContext());
        PathUtils.setPrivateDataDirectorySuffix(AwBrowserProcess.PRIVATE_DATA_DIRECTORY_SUFFIX);
        mSeedHolder = SeedHolder.getInstance();
        SeedFetcher.scheduleIfNeeded();
    }
}
