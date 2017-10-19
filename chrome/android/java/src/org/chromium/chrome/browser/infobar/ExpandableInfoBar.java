// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.support.v4.text.BidiFormatter;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * TODO
 */
public abstract class ExpandableInfoBar extends InfoBar {
    /** Whether the infobar should be shown as a mini-infobar or a classic expanded one. */
    private boolean mIsExpanded;

    /**
     * Constructor for expandable infobars.
     *
     * @param iconDrawableId ID of the resource to use for the Icon. If 0, no icon will be shown.
     */
    public ExpandableInfoBar(int iconDrawableId) {
        // |iconBitmap| has not been needed so far.
        // |message| is only used in the expanded infobar, which has been complex so far and does
        // not rely on the default builders, so it's unnecessary here too.
        super(iconDrawableId, null, null);
    }

    /** Returns the message to show when the infobar is in its compact mode. */
    protected abstract CharSequence getCompactMessage();

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        SpannableStringBuilder builder = new SpannableStringBuilder();
        builder.append(BidiFormatter.getInstance().unicodeWrap(getCompactMessage()));
        builder.append(" ");
        builder.append(makeDetailsLink());

        TextView prompt =
                (TextView) LayoutInflater.from(getContext())
                        .inflate(R.layout.infobar_expandable_compact_content, layout, false);
        prompt.setText(builder);
        prompt.setMovementMethod(LinkMovementMethod.getInstance());

        layout.addContent(prompt, 1f);
    }

    @Override
    protected boolean usesCompactLayout() {
        return !mIsExpanded;
    }

    @VisibleForTesting
    public void expand() {
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
                expand();
            }
        }, 0, label.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        return link;
    }
}
