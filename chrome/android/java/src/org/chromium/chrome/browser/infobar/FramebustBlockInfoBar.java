// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.text.SpannableString;
import android.text.Spanned;
import android.view.View;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.interventions.FramebustBlockMessageDelegate;
import org.chromium.chrome.browser.interventions.FramebustBlockMessageDelegateBridge;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * This InfoBar is shown to let the user know about a blocked Framebust and offer to
 * continue the redirection by tapping on a link.
 *
 * {@link FramebustBlockMessageDelegate} defines the messages shown in the infobar and
 * the target of the link.
 */
public class FramebustBlockInfoBar extends ExpandableInfoBar {
    private final FramebustBlockMessageDelegate mDelegate;

    @VisibleForTesting
    public FramebustBlockInfoBar(FramebustBlockMessageDelegate delegate) {
        super(delegate.getIconResourceId());
        mDelegate = delegate;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        assert isPrimaryButton;
        onCloseButtonClicked();
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(mDelegate.getLongMessage());

        // TODO(dgn): Elide the URL to fit on a single line.
        String link = UrlFormatter.formatUrlForSecurityDisplay(mDelegate.getBlockedUrl(), true);
        InfoBarControlLayout control = layout.addControlLayout();
        SpannableString text = new SpannableString(link);
        text.setSpan(new NoUnderlineClickableSpan() {
            @Override
            public void onClick(View view) {
                mDelegate.onLinkTapped();
            }
        }, 0, link.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        control.addDescription(text);

        layout.setButtons(getContext().getResources().getString(R.string.ok), null);
    }

    @Override
    protected CharSequence getCompactMessage() {
        return mDelegate.getShortMessage();
    }

    @CalledByNative
    private static FramebustBlockInfoBar create(long nativeFramebustBlockMessageDelegateBridge) {
        return new FramebustBlockInfoBar(
                new FramebustBlockMessageDelegateBridge(nativeFramebustBlockMessageDelegateBridge));
    }
}
