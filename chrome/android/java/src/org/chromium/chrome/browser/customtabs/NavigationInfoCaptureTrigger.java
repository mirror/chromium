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

    interface Delegate {
        boolean captureNavigationInfo(Tab tab);
    }

    NavigationInfoCaptureTrigger(Delegate delegate) {
        mDelegate = delegate;
        mUiThreadHandler = new Handler();
    }

    public void onNewNavigation() {
        mScreenshotTakenForCurrentNavigation = false;

        // Clear any Runnables that have yet to run.
        for (Runnable pending : mPendingRunnables) {
            mUiThreadHandler.removeCallbacks(pending);
        }
        mPendingRunnables.clear();
    }

    public void onHidden(Tab tab) {
        captureDelayed(tab, REDUCED_LOAD_DELAY_MS);
    }

    public void onLoadFinished(Tab tab) {
        captureDelayed(tab, ONLOAD_DELAY_MS);
    }

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
