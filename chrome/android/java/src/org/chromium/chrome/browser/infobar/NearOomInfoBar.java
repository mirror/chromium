// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.content.res.Resources;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;

/**
 * This InfoBar is shown to let the user know when the browser to action to stop a page from using
 * too much memory, potentially breaking it.
 * The default view is a compact infobar as FYI, that can be expanded to let the user decline the
 * intervention and let the page behave as it wants.
 *
 * <pre>
 * Compact layout
 * +----------------------------------------+
 * | ICON   COMPACT_MESSAGE Details.        |
 * +----------------------------------------+
 *
 * Expanded layout
 * +----------------------------------------+
 * | ICON   LONG_MESSAGE                    |
 * |                                        |
 * |          DECLINE_BUTTON  ACCEPT_BUTTON |
 * +----------------------------------------+
 * </pre>
 * The native caller can handle user action through {@code InfoBar::ProcessButton(int action)}
 */
public class NearOomInfoBar extends ExpandableInfoBar {
    private final String mLongMessage;
    private final String mCompactMessage;
    private final String mAcceptInterventionButtonText;
    private final String mDeclineInterventionButtonText;

    @VisibleForTesting
    public NearOomInfoBar() {
        super(R.drawable.infobar_chrome);
        Resources res = ContextUtils.getApplicationContext().getResources();
        mLongMessage = res.getString(R.string.near_oom_intervention_long_message);
        mCompactMessage = res.getString(R.string.near_oom_intervention_compact_message);
        mAcceptInterventionButtonText = res.getString(R.string.ok);
        mDeclineInterventionButtonText = res.getString(R.string.near_oom_intervention_decline);
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
