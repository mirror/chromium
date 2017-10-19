// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.support.v4.text.BidiFormatter;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.interventions.OomMessageDelegate;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * TODO
 */
public class OomInfoBar extends InfoBar {
    private final OomMessageDelegate mDelegate;

    /** Whether the infobar should be shown as a mini-infobar or a classic expanded one. */
    private boolean mIsExpanded;

    @VisibleForTesting
    public OomInfoBar(OomMessageDelegate delegate) {
        super(delegate.getIconResourceId(), null, null);
        mDelegate = delegate;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        if (isPrimaryButton) {
            onCloseButtonClicked();
        } else {
            mDelegate.escapeHatch();
        }
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(mDelegate.getMessage());

        layout.setButtons(
                getContext().getResources().getString(R.string.ok), mDelegate.getEscapeHatchText());
    }

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        final int messagePadding = getContext().getResources().getDimensionPixelOffset(
                R.dimen.reader_mode_infobar_text_padding);

        SpannableStringBuilder builder = new SpannableStringBuilder();
        builder.append(BidiFormatter.getInstance().unicodeWrap(mDelegate.getMessage()));
        builder.append(". ");
        builder.append(makeDetailsLink());

        TextView prompt = new TextView(getContext(), null);
        prompt.setText(builder);
        prompt.setMovementMethod(LinkMovementMethod.getInstance());
        prompt.setGravity(Gravity.CENTER_VERTICAL);
        prompt.setPadding(0, messagePadding, 0, messagePadding);

        layout.addContent(prompt, 1f);
    }

    @Override
    protected boolean usesCompactLayout() {
        return !mIsExpanded;
    }

    @Override
    public void onLinkClicked() {
        assert !mIsExpanded;
        mIsExpanded = true;
        replaceView(createView());
    }

    /**
     * Creates and sets up the "Details" link that allows going from the mini to the full infobar.
     */
    private SpannableString makeDetailsLink() {
        String label = getContext().getResources().getString(R.string.details_link);
        SpannableString link = new SpannableString(label);
        link.setSpan(new NoUnderlineClickableSpan() {
            @Override
            public void onClick(View view) {
                onLinkClicked();
            }
        }, 0, label.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        return link;
    }

    //    @CalledByNative
    //    private static OomInfoBar create(long nativeOomMessageDelegateBridge) {
    //        return new OomInfoBar(new OomMessageDelegateBridge(nativeOomMessageDelegateBridge));
    //    }
}
