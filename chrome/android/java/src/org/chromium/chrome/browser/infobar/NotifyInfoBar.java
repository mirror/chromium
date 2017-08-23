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
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * This InfoBar is shown to let the user know about something, and offers a link to follow, for
 * more info or as redirection for example.
 *
 * It has 2 states, tapping on a "Details" link switches from the compact state to the
 * expanded one. It also features an OK button that just closes the InfoBar.
 *
 * See {@link NotifyInfoBarDelegateBridge} to customize the infobar.
 */
public class NotifyInfoBar extends InfoBar {
    private final NotifyInfoBarDelegate mDelegate;

    /** Whether the infobar should show its compact layout. */
    private boolean mIsExpanded;

    @VisibleForTesting
    public NotifyInfoBar(NotifyInfoBarDelegate delegate) {
        super(delegate.getIconResourceId(), null, null);
        mDelegate = delegate;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        assert isPrimaryButton;
        onCloseButtonClicked();
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(mDelegate.getDescription());

        String link = mDelegate.getFeaturedLinkText();
        InfoBarControlLayout control = layout.addControlLayout();
        SpannableString text = new SpannableString(link);
        text.setSpan(new NoUnderlineClickableSpan() {
            @Override
            public void onClick(View view) {
                onLinkClicked();
            }
        }, 0, link.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        control.addDescription(text);

        layout.setButtons(getContext().getResources().getString(R.string.ok), null);
    }

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        final int messagePadding = getContext().getResources().getDimensionPixelOffset(
                R.dimen.reader_mode_infobar_text_padding);

        SpannableStringBuilder builder = new SpannableStringBuilder();
        builder.append(BidiFormatter.getInstance().unicodeWrap(mDelegate.getShortDescription()));
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
        if (!mIsExpanded) {
            mIsExpanded = true;
            replaceView(createView());
            return;
        }

        mDelegate.onLinkTapped();
    }

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

    @CalledByNative
    private static NotifyInfoBar create(long nativeNotifyInfoBarDelegateBridge) {
        return new NotifyInfoBar(
                new NotifyInfoBarDelegateBridge(nativeNotifyInfoBarDelegateBridge));
    }
}
