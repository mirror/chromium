// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.text;

import android.text.TextPaint;
import android.text.style.ClickableSpan;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;

/**
* Show a clickable link with underlines turned off.
*/
public abstract class NoUnderlineClickableSpan extends ClickableSpan {
    private boolean mShouldSetColor;
    private int mColor;

    public NoUnderlineClickableSpan() {
        super();
    }

    /**
     * @param colorId The color resource id to be set in updateDrawState.
     */
    public NoUnderlineClickableSpan(int colorId) {
        super();
        mShouldSetColor = true;
        mColor = ApiCompatibilityUtils.getColor(
                ContextUtils.getApplicationContext().getResources(), colorId);
    }

    // Disable underline on the link text.
    @Override
    public void updateDrawState(TextPaint textPaint) {
        super.updateDrawState(textPaint);
        textPaint.setUnderlineText(false);
        if (mShouldSetColor) textPaint.setColor(mColor);
    }
}
