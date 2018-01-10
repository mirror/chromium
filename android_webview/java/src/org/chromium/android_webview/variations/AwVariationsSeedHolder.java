// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.os.ConditionVariable;

import org.chromium.android_webview.AwVariationsUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.util.concurrent.locks.ReentrantReadWriteLock;

// A singleton which manages the local copy of the variations seed - both the file and the SeedInfo
// which is the data loaded from that file - in WebView's Finch server process. The seed is served
// to apps by AwVariationsSeedServer and updated AwVariationsSeedFetcher. AwVariationsSeedHolder
// guards concurrent access by those classes' Binder and AsyncTask threads. It has "package" access
// because it's not meant to be used outside the service.
/* package */ class AwVariationsSeedHolder {
    private static final String TAG = "AwVariationsSeedHo-";

    private static AwVariationsSeedHolder sInstance;

    // mSeed is the seed data loaded from mSeedFile. mSeed is initially null, until the first
    // successful loadSeed() or updateSeed(). Seed data then remains loaded for the remaining
    // process lifetime. mSeed is always up-to-date with the seed file; they are updated together in
    // updateSeed() when a new seed is downloaded.
    private SeedInfo mSeed;
    private File mSeedFile = AwVariationsUtils.getSeedFile();
    // mSeedLock protects both mSeed and the file that mSeedFile refers to. Only used in the Finch
    // server process, not in apps using WebView.
    private ReentrantReadWriteLock mSeedLock = new ReentrantReadWriteLock();
    // Threads waiting for mSeed to become non-null will wait on mHaveSeed. It will be opened after
    // the seed is loaded, and will then remain open for the remaining process lifetime.
    private ConditionVariable mHaveSeed = new ConditionVariable(false);

    /* package */ static AwVariationsSeedHolder getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) sInstance = new AwVariationsSeedHolder();
        return sInstance;
    }

    private AwVariationsSeedHolder() {}

    // Write the held seed to the given fd, blocking until the held seed is available for reading,
    // or lazily loading the seed if it hasn't been loaded yet.
    /* package */ void writeSeedToFd(FileDescriptor fd) {
        ThreadUtils.assertOnBackgroundThread();

        if (mSeed == null) loadSeed();

        mSeedLock.readLock().lock();
        try {
            AwVariationsUtils.writeSeedToStream(new FileOutputStream(fd), mSeed);
        } finally {
            mSeedLock.readLock().unlock();
        }
    }

    // Set mSeed to newSeed and replace mSeedFile with newSeedFile, From the point of view of anyone
    // reading the seed (and taking the read lock), mSeed should always accurately represent the
    // contents of mSeedFile. This is ensured by taking the write lock and then updating mSeed and
    // mSeedFile together. This is to prevent a race condition between loadSeed() and updateSeed()
    // where loadSeed returns the old seed data, even though updateSeed() has provided new data.
    /* package */ void updateSeed(SeedInfo newSeed, File newSeedFile) {
        ThreadUtils.assertOnBackgroundThread();
        assert newSeed != null;

        mSeedLock.writeLock().lock();
        try {
            if (!newSeedFile.renameTo(mSeedFile)) {
                Log.e("blarg", "Failed to swap out old seed " + mSeedFile +
                        " with new seed " + newSeedFile);
                return;
            }
            mSeed = newSeed;
            mHaveSeed.open();
        } finally {
            mSeedLock.writeLock().unlock();
        }
    }

    // Load the seed on behalf of the Finch service. May be called concurrently on multiple threads;
    // tryLock ensures only one thread will actually load the seed. Other threads will wait on
    // mSeedLock for loading to finish.
    /* package */ void loadSeed() {
        ThreadUtils.assertOnBackgroundThread();

        if (mSeedLock.writeLock().tryLock()) {
            try {
                if (mSeed != null) return; // A concurrent loadSeed() may have beat us to it.

                mSeed = AwVariationsUtils.readSeedFile(mSeedFile);

                // If loading succeeded, release the threads waiting on the seed.
                if (mSeed != null) mHaveSeed.open();
            } finally {
                mSeedLock.writeLock().unlock();
            }
        }

        // At this point there are 3 possibilities:
        // 1. We took the write lock. We successfully loaded the seed and opened mHaveSeed, so
        //    block() will no-op.
        // 2. We took the write lock. We tried and failed to load the seed. Block on mHaveSeed to
        //    join the other threads in waiting for a seed. We may all be released later by a
        //    subsequent loadSeed() or updateSeed(), or the service may die before then.
        // 3. Another thread beat us to the write lock. We should wait until they've finished by
        //    blocking on mHaveSeed. (If they already finished, block() will no-op.)
        mHaveSeed.block();
    }
}
