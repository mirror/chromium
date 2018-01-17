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
 * Navigation information contains a screenshot which ideally would show a good representation of
 * the page. We cannot simply capture this after the page's onload has triggered since many slower
 * pages haven't fully loaded at this point, and the screenshot would be blank.
 *
 * To try to get a screenshot once the page is showing something useful, we wait for the first of:
 * * A long delay after page load.
 * * The first contentful paint. This itself is a heuristic primarily used for metrics.
 * Finally, if neither of these have fired, we capture when the Tab is hidden. This essentially
 * boils down to triggering on the first of any of these events (and their delays).
 *
 * The first contentful paint and tab hidden triggers occur during processor intensive times, so we
 * add a slight delay to each of these before capturing the screenshot.
 */
public class NavigationInfoCaptureTrigger {
    /** A delay added to not capture navigation info during processor busy times. */
    private static final int REDUCED_LOAD_DELAY_MS = 200;
    /** A delay added after page onload has been reported for the page to show something useful. */
    private static final int ONLOAD_DELAY_MS = 5000;

    private final List<Runnable> mPendingRunnables = new LinkedList<>();
    private final Delegate mDelegate;
    private final Handler mUiThreadHandler;

    private boolean mScreenshotTakenForCurrentNavigation;

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

    NavigationInfoCaptureTrigger(Delegate delegate) {
        mDelegate = delegate;
        mUiThreadHandler = new Handler();
    }

    /** Notifies that the Tab has navigated. */
    public void onNewNavigation() {
        mScreenshotTakenForCurrentNavigation = false;

        // Clear any Runnables that have yet to run.
        for (Runnable pending : mPendingRunnables) {
            mUiThreadHandler.removeCallbacks(pending);
        }
        mPendingRunnables.clear();
    }

    /** Notifies that the Tab has hidden. */
    public void onHidden(Tab tab) {
        captureDelayed(tab, REDUCED_LOAD_DELAY_MS);
    }

    /** Notifies that the Tab has finished loading its page.*/
    public void onLoadFinished(Tab tab) {
        captureDelayed(tab, ONLOAD_DELAY_MS);
    }

    /** Notifies that the Tab has had its first contenful paint. */
    public void onFirstContentfulPaint(Tab tab) {
        captureDelayed(tab, REDUCED_LOAD_DELAY_MS);
    }

    private void captureDelayed(Tab tab, int delay) {
        if (mScreenshotTakenForCurrentNavigation) return;

        Runnable runnable = () -> {
            // Check again since things may have changed in the meantime.
            if (mScreenshotTakenForCurrentNavigation) return;

            if (mDelegate.captureNavigationInfo(tab)) {
                mScreenshotTakenForCurrentNavigation = true;
                mPendingRunnables.remove(this);
            }
        };

        mUiThreadHandler.postDelayed(runnable, delay);
        mPendingRunnables.add(runnable);
    }
}
