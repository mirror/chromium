// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import org.chromium.android_webview.AwBrowserProcess;

import android.app.Service;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;

import java.io.FileDescriptor;
import java.util.concurrent.RejectedExecutionException;

/**
 * AwVariationsSeedServer is a bound service that shares the Variations seed with all the WebViews
 * on the system. A WebView will bind and call getSeed, passing a file descriptor to which the
 * service should write the seed.
 */
public class AwVariationsSeedServer extends Service {
    private static final String TAG = "AwVariationsSeedSe-";

    private AwVariationsSeedHolder mSeedHolder;

    private final IAwVariationsSeedServer.Stub mBinder = new IAwVariationsSeedServer.Stub() {
        @Override
        public void getSeed(ParcelFileDescriptor dest) {
            final FileDescriptor fd = dest.getFileDescriptor();
            try {
                (new AsyncTask<Void, Void, Void>() {
                    @Override
                    protected Void doInBackground(Void... unused) {
                        mSeedHolder.writeSeedToFd(fd);
                        return null;
                    }
                }).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            } catch (RejectedExecutionException e) {
                // This happens if we max out THREAD_POOL_EXECUTOR's internal task queue. The queue
                // is large, so this shouldn't normally happen. If it does, just give up.
                Log.e("blarg", "Too many seed requests - dropping request from PID " +
                        getCallingPid());
            }
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
        mSeedHolder = AwVariationsSeedHolder.getInstance();
        AwVariationsSeedFetcher.scheduleNeededJobs(AwVariationsSeedFetcher.NO_JOB_ID);
    }
}
