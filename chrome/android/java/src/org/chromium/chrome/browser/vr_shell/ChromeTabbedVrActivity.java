// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.content.pm.ActivityInfo;
import android.view.View;

import com.google.vr.ndk.base.AndroidCompat;

import org.chromium.chrome.browser.ChromeTabbedActivity;

/**
 * TODO(ymalik): Add JavaDOC.
 */
public class ChromeTabbedVrActivity
        extends ChromeTabbedActivity implements View.OnSystemUiVisibilityChangeListener {
    private boolean mVrModeExpected;
    private Integer mRestoreSystemUiVisibilityFlag = null;
    private Integer mRestoreOrientation = null;

    private static final int HIDE_SYSTEM_UI_FLAGS =
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN;

    @Override
    public void preInflationStartup() {
        if (VrShellDelegate.isVrIntent(getIntent())) {
            View view = getWindow().getDecorView();
            AndroidCompat.setVrModeEnabled(this, true);
            mVrModeExpected = true;
            view.setOnSystemUiVisibilityChangeListener(this);

            // Save window mode.
            mRestoreOrientation = getRequestedOrientation();
            mRestoreSystemUiVisibilityFlag = getWindow().getDecorView().getSystemUiVisibility();
            // Set VR specific window mode.
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
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
        setRequestedOrientation(mRestoreOrientation);
    }
}
