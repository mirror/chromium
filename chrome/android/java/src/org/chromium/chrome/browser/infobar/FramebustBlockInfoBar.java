// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.support.annotation.StringRes;
import android.support.v4.text.BidiFormatter;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * This InfoBar is shown to let the user know that Chrome blocked a framebust attempt.
 */
public class FramebustBlockInfoBar extends InfoBar {
    private final String mBlockedUrl;

    /** Whether the infobar should show its compact layout. */
    private boolean mIsExpanded;

    /**
     * Create and show the framebust block {@link InfoBar}.
     * @param tab The tab that the {@link InfoBar} should be shown in.
     * @param url The url the framebust tried to redirect to.
     */
    public static void showFramebustBlockInfoBar(Tab tab, String url) {
        nativeCreate(tab, url);
    }

    private FramebustBlockInfoBar(String blockedUrl) {
        super(R.drawable.infobar_chrome, null, null);
        mBlockedUrl = blockedUrl;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        assert isPrimaryButton;
        onCloseButtonClicked();
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(getString(R.string.redirect_blocked_infobar_description));

        String link = mBlockedUrl;
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
        builder.append(BidiFormatter.getInstance().unicodeWrap(
                getString(R.string.redirect_blocked_infobar_short_description)));
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

        // Delegate the link action to be handled by the native side.
        super.onLinkClicked();
    }

    private SpannableString makeDetailsLink() {
        String label = getString(R.string.details_link);
        SpannableString link = new SpannableString(label);
        link.setSpan(new NoUnderlineClickableSpan() {
            @Override
            public void onClick(View view) {
                onLinkClicked();
            }
        }, 0, label.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        return link;
    }

    private String getString(@StringRes int resId) {
        return getContext().getResources().getString(resId);
    }

    /**
     * @return An instance of the {@link FramebustBlockInfoBar}.
     */
    @CalledByNative
    private static FramebustBlockInfoBar create(String blockedUrl) {
        return new FramebustBlockInfoBar(blockedUrl);
    }

    private static native void nativeCreate(Tab tab, String url);
}
