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

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * This Infobar is shown to let the user know that Chrome blocked a framebust attempt.
 */
public class FramebustBlockInfoBar extends InfoBar {
    private final long mNativeInfoBar;
    private boolean mIsExpanded;

    /**
     * Create and show the Reader Mode {@link InfoBar}.
     * @param tab The tab that the {@link InfoBar} should be shown in.
     */
    public static void showFramebustBlockInfoBar(Tab tab, String url) {
        nativeCreate(tab, url);
    }

    /**
     * Default constructor.
     */
    private FramebustBlockInfoBar(long nativeInfoBar) {
        super(R.drawable.infobar_chrome, null, null);
        mNativeInfoBar = nativeInfoBar;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        assert isPrimaryButton;
        onCloseButtonClicked();
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(nativeGetDescription(mNativeInfoBar));

        String link = nativeGetLink(mNativeInfoBar);
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
        builder.append(
                BidiFormatter.getInstance().unicodeWrap(nativeGetShortDescription(mNativeInfoBar)));
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
        super.onLinkClicked();
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

    /**
     * @return An instance of the {@link FramebustBlockInfoBar}.
     */
    @CalledByNative
    private static FramebustBlockInfoBar create(long nativeInfoBar) {
        return new FramebustBlockInfoBar(nativeInfoBar);
    }

    private static native void nativeCreate(Tab tab, String url);
    private native String nativeGetDescription(long nativeFramebustBlockInfoBar);
    private native String nativeGetShortDescription(long nativeFramebustBlockInfoBar);
    private native String nativeGetLink(long nativeFramebustBlockInfoBar);
}
