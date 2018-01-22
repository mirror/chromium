// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import android.os.Handler;
import android.support.annotation.Nullable;

import org.chromium.chrome.browser.tab.Tab;

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

    private final Delegate mDelegate;
    private final Handler mUiThreadHandler = new Handler();


    private final Capture mInitialCapture = new Capture();
    private final Capture mBackupCapture = new Capture();
    private final Capture mFirstMeaningfulPaintCapture = new Capture();

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
    }

    /** Notifies that the Tab has finished loading its page.*/
    public void onLoadFinished(Tab tab) {
        mInitialCapture.maybeSchedule(tab, INITIAL_CAPTURE_DELAY_MS, null);
        mBackupCapture.maybeSchedule(tab, BACKUP_CAPTURE_DELAY_MS, () -> {
            mFirstMeaningfulPaintCapture.cancel();
        });
    }

    /** Notifies that the Tab has had its first meaningful paint. */
    public void onFirstMeaningfulPaint(Tab tab) {
        mFirstMeaningfulPaintCapture.maybeSchedule(tab, FMP_CAPTURE_DELAY_MS, () -> {
            mInitialCapture.cancel();
            mBackupCapture.cancel();
        });
    }

    /** A class that schedules capturing navigation info. */
    private class Capture {
        private boolean mCancelled;
        @Nullable private Runnable mQueuedRunnable;

        public void maybeSchedule(Tab tab, int delay, @Nullable Runnable postCapture) {
            if (mCancelled) return;

            mQueuedRunnable = () -> {
                // Check again since things may have changed in the meantime.
                if (mCancelled) return;

                if (mDelegate.captureNavigationInfo(tab) && postCapture != null) {
                    postCapture.run();
                }

                // Don't let the same trigger be run twice for the same navigation.
                cancel();

                mQueuedRunnable = null;
            };

            mUiThreadHandler.postDelayed(mQueuedRunnable, delay);
        }

        public void reset() {
            mCancelled = false;

            clearRunnable();
        }

        public void cancel() {
            mCancelled = true;

            clearRunnable();
        }

        private void clearRunnable() {
            if (mQueuedRunnable == null) return;
            mUiThreadHandler.removeCallbacks(mQueuedRunnable);
            mQueuedRunnable = null;
        }
    }
}
