// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.os.StrictMode;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.tabmodel.TabPersistencePolicy;
import org.chromium.chrome.browser.tabmodel.TabPersistentStore;

import java.io.File;
import java.util.List;
import java.util.concurrent.Executor;

import javax.annotation.Nullable;

/**
 * Handles the Browser Actions Tab specific behaviors of tab persistence.
 */
public class BrowserActionsTabPersistencePolicy implements TabPersistencePolicy {
    static final String SAVED_STATE_DIRECTORY = "0";

    private static final String TAG = "tabmodel";

    /** Prevents two state directories from getting created simultaneously. */
    private static final Object DIR_CREATION_LOCK = new Object();

    private static File sStateDirectory;

    @Override
    public File getOrCreateStateDirectory() {
        return getOrCreateBrowserActionsModeStateDirectory();
    }

    @Override
    public String getStateFileName() {
        return TabPersistentStore.getStateFileName("2");
    }

    @Override
    public boolean shouldMergeOnStartup() {
        return false;
    }

    @Override
    @Nullable
    public String getStateToBeMergedFileName() {
        return null;
    }

    @Override
    public boolean performInitialization(Executor executor) {
        return true;
    }

    @Override
    public void waitForInitializationToFinish() {}

    @Override
    public boolean isMergeInProgress() {
        return false;
    }

    @Override
    public void setMergeInProgress(boolean isStarted) {
        assert false : "Merge not supported in Browser Actions";
    }

    @Override
    public void cancelCleanupInProgress() {}

    @Override
    public void cleanupUnusedFiles(Callback<List<String>> filesToDelete) {}

    @Override
    public void setTabContentManager(TabContentManager cache) {}

    @Override
    public void notifyStateLoaded(int tabCountAtStartup) {}

    @Override
    public void destroy() {}

    /**
     * The folder where the state should be saved to.
     * @return A file representing the directory that contains TabModelSelector states.
     */
    public static File getOrCreateBrowserActionsModeStateDirectory() {
        synchronized (DIR_CREATION_LOCK) {
            if (sStateDirectory == null) {
                sStateDirectory = new File(
                        TabPersistentStore.getOrCreateBaseStateDirectory(), SAVED_STATE_DIRECTORY);
                StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
                try {
                    if (!sStateDirectory.exists() && !sStateDirectory.mkdirs()) {
                        Log.e(TAG, "Failed to create state folder: " + sStateDirectory);
                    }
                } finally {
                    StrictMode.setThreadPolicy(oldPolicy);
                }
            }
        }
        return sStateDirectory;
    }
}
