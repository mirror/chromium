// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import android.os.Handler;

import org.chromium.chrome.browser.tab.Tab;

import java.util.LinkedList;
import java.util.List;

/**
 * This class contains logic for capturing navigation info at an appropriate time.
 *
 * There are three triggers this class considers:
 * - Initial - a short time after a page's onload.
 * - Backup - a longer time after a page's onload.
 * - First Meaningful Paint (FMP) - a medium time after a page's first meaningful paint occurs.
 *
 * The first meaningful paint is the best signal we have to determine that the page is in a usable
 * state, but with some caveats:
 * - It happens after a few seconds so the user may load a page then exit before it occurs. We use
 * the 'initial' trigger to provide a poor but "better than nothing" capture in this case.
 * - It may not trigger at all. We use the 'backup' trigger to provide a good capture here.
 * - It may occur before onload.
 *
 * Therefore, we capture on the 'initial' trigger (unless FMP has already occurred), and then
 * on the first of 'backup' or 'FMP'.
 */
public class NavigationInfoCaptureTrigger {
    private static final int INITIAL_CAPTURE_DELAY_MS = 1000;
    private static final int BACKUP_CAPTURE_DELAY_MS = 10000;
    private static final int FMP_CAPTURE_DELAY_MS = 3000;

    private final List<Runnable> mPendingRunnables = new LinkedList<>();
    private final Delegate mDelegate;
    private final Handler mUiThreadHandler = new Handler();

    /**
     * A delegate that the {@link NavigationInfoCaptureTrigger} will call when it determines it is
     * appropriate to capture the navigation info.
     */
    public interface Delegate {
        /**
         * Capture the navigation info. Returns false if navigation info could not be captured and
         * should be attempted again later.
         */
        boolean captureNavigationInfo(Tab tab);
    }

    public NavigationInfoCaptureTrigger(Delegate delegate) {
        mDelegate = delegate;
    }

    /** Notifies that the Tab has navigated. */
    public void onNewNavigation() {
        mInitialCapture.reset();
        mBackupCapture.reset();
        mFirstMeaningfulPaintCapture.reset();

        // Clear any Runnables that have yet to run.
        for (Runnable pending : mPendingRunnables) {
            mUiThreadHandler.removeCallbacks(pending);
        }
        mPendingRunnables.clear();
    }

    /** Notifies that the Tab has finished loading its page.*/
    public void onLoadFinished(Tab tab) {
        mInitialCapture.schedule(tab);
        mBackupCapture.schedule(tab);
    }

    /** Notifies that the Tab has had its first meaningful paint. */
    public void onFirstMeaningfulPaint(Tab tab) {
        mFirstMeaningfulPaintCapture.schedule(tab);
    }

    /** A class that schedules capturing navigation info. */
    private abstract class Capture {
        private boolean mCancelled;
        private int mDelayMs;

        Capture(int delayMs) {
            mDelayMs = delayMs;
        }

        int getDelayMs() {
            return mDelayMs;
        }

        void schedule(Tab tab) {
            if (mCancelled) return;

            Runnable runnable = () -> {
                // Check again since things may have changed in the meantime.
                if (mCancelled) return;

                if (mDelegate.captureNavigationInfo(tab)) {
                    postCapture();

                    // Don't let the same trigger be run twice for the same navigation.
                    cancel();

                    mPendingRunnables.remove(this);
                }
            };

            mPendingRunnables.add(runnable);
            mUiThreadHandler.postDelayed(runnable, mDelayMs);
        }
        void reset() {
            mCancelled = false;
        }
        void cancel() {
            mCancelled = true;
        }
        abstract void postCapture();
    }

    private final Capture mInitialCapture = new Capture(INITIAL_CAPTURE_DELAY_MS) {
        @Override
        void postCapture() { }
    };

    private final Capture mBackupCapture = new Capture(BACKUP_CAPTURE_DELAY_MS) {
        @Override
        void postCapture() {
            mFirstMeaningfulPaintCapture.cancel();
        }
    };

    private final Capture mFirstMeaningfulPaintCapture = new Capture(FMP_CAPTURE_DELAY_MS) {
        @Override
        void postCapture() {
            mInitialCapture.cancel();
            mBackupCapture.cancel();
        }
    };
}
