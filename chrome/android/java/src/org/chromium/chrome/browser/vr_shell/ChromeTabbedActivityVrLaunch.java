// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.view.View;

import com.google.vr.ndk.base.AndroidCompat;

import org.chromium.chrome.browser.ChromeTabbedActivity;

/**
 * A subclass of ChromeTabbedActivity created when cold-starting starting Chrome in VR mode.
 *
 * The main purpose of this activity is to add flexibility to the way Chrome is started when the
 * user's phone is already in their VR headset (e.g, we want to hide the System UI).
 *
 * This activity behaves identical to ChromeTabbedActivity once the user exits VR mode. That is, it
 * shares the same tab state (e.g. adding and modifying the same set of tabs). For that reason, this
 * activity should never coexist with ChromeTabbedActivity.
 *
 * See {@link MultiWindowUtil#getTabbedActivityForIntent} for more details on how intents are
 * handled when this activity is running.
 */
public class ChromeTabbedActivityVrLaunch
        extends ChromeTabbedActivity implements View.OnSystemUiVisibilityChangeListener {
    private boolean mVrModeExpected;
    private Integer mRestoreSystemUiVisibilityFlag = null;

    private static final int HIDE_SYSTEM_UI_FLAGS =
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN;

    @Override
    public void preInflationStartup() {
        if (getSavedInstanceState() == null
                && VrIntentHandler.getInstance().isTrustedDaydreamIntent(getIntent())) {
            View view = getWindow().getDecorView();
            AndroidCompat.setVrModeEnabled(this, true);
            mVrModeExpected = true;
            view.setOnSystemUiVisibilityChangeListener(this);

            // Save window mode.
            mRestoreSystemUiVisibilityFlag = getWindow().getDecorView().getSystemUiVisibility();
            // Set VR specific window mode.
            view.setSystemUiVisibility(VrShellDelegate.VR_SYSTEM_UI_FLAGS);
        }
        super.preInflationStartup();
    }

    @Override
    public void onSystemUiVisibilityChange(int visibility) {
        mRestoreSystemUiVisibilityFlag = HIDE_SYSTEM_UI_FLAGS ^ visibility;
    }

    @Override
    public void onEnterVr() {
        super.onEnterVr();
        getWindow().getDecorView().setOnSystemUiVisibilityChangeListener(null);
    }

    @Override
    public void onExitVr() {
        super.onExitVr();
        AndroidCompat.setVrModeEnabled(this, false);
        assert mVrModeExpected;

        // Restore window mode.
        getWindow().getDecorView().setSystemUiVisibility(mRestoreSystemUiVisibilityFlag);
    }
}
