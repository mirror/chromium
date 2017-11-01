// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.text.style.ClickableSpan;

/**
 * Observer for survey info bar actions.
 */
public class SurveyInfoBarDelegate {

    /**
     * Notified when the survey infobar is closed.
     */
    public void onSurveyInfoBarClosed() {}

    /**
     * Notified when the survey is triggered via the infobar.
     */
    public void onSurveyTriggered() {}

    /**
     * Called to supply the survey info bar with the prompt string.
     * @param clickableSpan The clickable span containing the onClick method.
     * @return The CharSequence which will appear on the infobar.
     *         The clickable span should be included.
     */
    public CharSequence getSurveyPromptString(ClickableSpan clickableSpan) {
        return null;
    }
}