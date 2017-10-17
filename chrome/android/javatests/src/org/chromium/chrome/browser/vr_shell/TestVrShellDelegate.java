// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.os.Handler;

import org.junit.Assert;

import org.chromium.chrome.browser.ChromeActivity;

/**
 * Class for accessing VrShellDelegate internals for testing purposes.
 * This does two things:
 * - Prevents us from needing @VisibleForTesting annotations everywhere in production code
 * - Allows us to have test-specific behavior if necessary without changing production code
 */
public class TestVrShellDelegate extends VrShellDelegate {
    // Arbitrary but valid delay to make sure that we actually did cancel the DON flow instead of
    // running into crbug.com/762724
    private static final int DON_CANCEL_DELAY_MS = 200;

    public TestVrShellDelegate(ChromeActivity activity, VrClassesWrapper wrapper) {
        super(activity, wrapper);
    }

    /**
     * Sets the VrShellDelegate instance constructor to point to this class.
     *
     * This should not be called from within any tests, as it is already called before a test case
     * is run during Rule application.
     */
    public static void overrideBaseDelegateConstructor() {
        try {
            VrShellDelegate.setInstanceConstructor(TestVrShellDelegate.class.getConstructor(
                    ChromeActivity.class, VrClassesWrapper.class));
        } catch (NoSuchMethodException | SecurityException e) {
            // Shouldn't ever happen?
            Assert.fail(e.toString());
        }
    }

    public static TestVrShellDelegate getInstanceForTesting() {
        return (TestVrShellDelegate) VrShellDelegate.getInstance();
    }

    public static boolean isDisplayingUrlForTesting() {
        return VrShellDelegate.isDisplayingUrl();
    }

    public static VrShell getVrShellForTesting() {
        return VrShellDelegate.getVrShell();
    }

    public void shutdownVr(boolean disableVrMode, boolean stayingInChrome) {
        super.shutdownVr(disableVrMode, stayingInChrome);
    }

    public void overrideDaydreamApiForTesting(VrDaydreamApi api) {
        super.overrideDaydreamApi(api);
    }

    public void overrideVrCoreVersionCheckerForTesting(VrCoreVersionChecker versionChecker) {
        super.overrideVrCoreVersionChecker(versionChecker);
    }

    public void setFeedbackFrequencyForTesting(int frequency) {
        super.setFeedbackFrequency(frequency);
    }

    public boolean isListeningForWebVrActivate() {
        return super.isListeningForWebVrActivate();
    }

    public boolean isClearActivatePending() {
        return super.isClearActivatePending();
    }

    public boolean isVrEntryComplete() {
        return super.isVrEntryComplete();
    }

    /**
     * Wait a short period of time before running if we think the DON flow was cancelled.
     *
     * The same as the production onResume, except that in the case of mProbablyInDon still being
     * set, the decision that the DON flow was cancelled is delayed until later to see if the
     * broadcast is just late. This is caused by crbug.com/762724.
     * TODO(bsheedy): Remove this when the root cause is fixed.
     */
    @Override
    protected void onResume() {
        if (!mDonSucceeded && mProbablyInDon) {
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    TestVrShellDelegate.super.onResume();
                }
            }, DON_CANCEL_DELAY_MS);
        } else {
            super.onResume();
        }
    }
}