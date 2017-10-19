// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;

/**
 * TODO
 */
public class NearOomInfoBar extends ExpandableInfoBar {
    private final String mLongMessage;
    private final String mCompactMessage;
    private final String mAcceptInterventionButtonText;
    private final String mDeclineInterventionButtonText;

    @VisibleForTesting
    public NearOomInfoBar() {
        super(R.drawable.infobar_chrome);
        mLongMessage = "This page uses too much memory, so Chrome paused it";
        mCompactMessage = "This page uses too much memory, so Chrome paused it.";
        mAcceptInterventionButtonText = "OK";
        mDeclineInterventionButtonText = "Show original";
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        onButtonClicked(isPrimaryButton ? ActionType.OK : ActionType.CANCEL);
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(mLongMessage);
        layout.setButtons(mAcceptInterventionButtonText, mDeclineInterventionButtonText);
    }

    @Override
    protected CharSequence getCompactMessage() {
        return mCompactMessage;
    }

    @CalledByNative
    private static NearOomInfoBar create() {
        return new NearOomInfoBar();
    }
}
