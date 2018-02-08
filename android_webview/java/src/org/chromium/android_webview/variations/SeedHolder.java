// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.ParcelFileDescriptor;
import android.os.ParcelFileDescriptor.AutoCloseOutputStream;

import org.chromium.android_webview.VariationsUtils;
import org.chromium.base.Log;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.util.ArrayList;

/**
 * SeedHolder is a singleton which manages the local copy of the variations seed - both the file and
 * the SeedInfo object containing the data loaded from that file - in WebView's Finch server
 * process. SeedHolder is used by SeedServer (to serve the seed to apps) and SeedFetcher (to update
 * the seed). SeedHolder guards concurrent access to the seed by serializing all operations onto
 * mSeedThread. SeedHolder has package (rather than public) access because it's not meant to be used
 * outside the service.
 */
/* package */ class SeedHolder {
    private static final String TAG = "SeedHolder";

    private static SeedHolder sInstance;

    // mSeed and mDeferredWrites should only be used on mSeedThread. Send tasks to mSeedThread via
    // mSeedHandler.
    private HandlerThread mSeedThread;
    private Handler mSeedHandler;
    private SeedInfo mSeed;
    private ArrayList<ParcelFileDescriptor> mDeferredWrites;

    // A Runnable which handles an individual request for the seed. Must run on mSeedThread.
    private class SeedWriter implements Runnable {
        private ParcelFileDescriptor mDest;

        public SeedWriter(ParcelFileDescriptor dest) {
            mDest = dest;
        }

        @Override
        public void run() {
            assert Thread.currentThread() == mSeedThread;

            if (mSeed == null) {
                mSeed = VariationsUtils.readSeedFile(VariationsUtils.getSeedFile());

                // If reading failed (most likely because no seed has been downloaded yet) defer
                // this work for when the seed becomes available (when SeedUpdater runs). It's
                // possible the service won't survive that long, and the deferred work will be
                // dropped. In that case, the requesting apps will have to request again.
                if (mSeed == null) {
                    if (mDeferredWrites == null) {
                        mDeferredWrites = new ArrayList<ParcelFileDescriptor>();
                    }
                    mDeferredWrites.add(mDest);
                    return;
                }
            }

            // Ignore the return status because if it fails, the app will ignore the bad seed and
            // request a new one.
            VariationsUtils.writeSeedToStream(new AutoCloseOutputStream(mDest), mSeed);
        }
    }

    // A Runnable which updates both mSeed and the service's seed file. Must run on mSeedThread.
    private class SeedUpdater implements Runnable {
        private SeedInfo mNewSeed;
        private Runnable mOnFinished;

        public SeedUpdater(SeedInfo newSeed, Runnable onFinished) {
            mNewSeed = newSeed;
            mOnFinished = onFinished;
        }

        @Override
        public void run() {
            assert Thread.currentThread() == mSeedThread;
            try {
                mSeed = mNewSeed;

                // Update the seed file.
                File newSeedFile = VariationsUtils.getNewSeedFile();
                FileOutputStream out;
                try {
                    out = new FileOutputStream(newSeedFile);
                } catch (FileNotFoundException e) {
                    Log.e(TAG, "Failed to open seed file " + newSeedFile + " for update");
                    return;
                }
                if (!VariationsUtils.writeSeedToStream(out, mSeed)) {
                    Log.e(TAG, "Failed to write seed file " + newSeedFile + " for update");
                    return;
                }
                VariationsUtils.replaceOldWithNewSeed();

                // If there were any requests waiting for the seed, handle them now.
                if (mDeferredWrites != null) {
                    for (ParcelFileDescriptor pfd : mDeferredWrites) {
                        VariationsUtils.writeSeedToStream(new AutoCloseOutputStream(pfd), mSeed);
                    }
                    mDeferredWrites = null;
                }
            } finally {
                mOnFinished.run();
            }
        }
    }

    private SeedHolder() {
        mSeedThread = new HandlerThread(/*name=*/"seed_holder");
        mSeedThread.start();
        mSeedHandler = new Handler(mSeedThread.getLooper());
    }

    public static SeedHolder getInstance() {
        if (sInstance == null) {
            sInstance = new SeedHolder();
        }
        return sInstance;
    }

    public void writeSeed(ParcelFileDescriptor dest) {
        mSeedHandler.post(new SeedWriter(dest));
    }

    public void updateSeed(SeedInfo newSeed, Runnable onFinished) {
        mSeedHandler.post(new SeedUpdater(newSeed, onFinished));
    }
}
