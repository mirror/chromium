// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.net.Uri;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.components.url_formatter.UrlFormatter;

/**
 * This InfoBar is shown to let the user know about blocked popups and offer to
 * continue the popup (and whitelist popups on the site).
 */
public class PopupBlockInfoBar extends InfoBar {
    private final String[] mBlockedUrls;

    /** Whether the infobar should be shown as a mini-infobar or a classic expanded one. */
    private boolean mIsExpanded;

    @VisibleForTesting
    public PopupBlockInfoBar(String[] blockedUrls) {
        super(R.drawable.infobar_blocked_popups, null, null);
        mBlockedUrls = blockedUrls;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        assert isPrimaryButton;
        onButtonClicked(ActionType.OK);
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(getString(R.string.blocked_popups_message));
        InfoBarControlLayout control = layout.addControlLayout();

        // Cap to the last 3 URLs.
        int startIndex = 0;
        if (mBlockedUrls.length > 3)
          startIndex = mBlockedUrls.length - 3;
        for (int i = startIndex; i < mBlockedUrls.length; ++i) {
            String blockedUrl = mBlockedUrls[i];
            ViewGroup ellipsizerView =
                    (ViewGroup) LayoutInflater.from(getContext())
                            .inflate(R.layout.infobar_control_url_ellipsizer, control, false);

            // Formatting the URL and requesting to omit the scheme might still include it for some
            // of them (e.g. file, filesystem). We split the output of the formatting to make sure
            // we don't end up duplicating it.
            String formattedUrl = UrlFormatter.formatUrlForSecurityDisplay(blockedUrl, true);
            String scheme = Uri.parse(blockedUrl).getScheme() + "://";

            TextView schemeView = ellipsizerView.findViewById(R.id.url_scheme);
            schemeView.setText(scheme);

            TextView urlView = ellipsizerView.findViewById(R.id.url_minus_scheme);
            urlView.setText(formattedUrl.substring(scheme.length()));

            ellipsizerView.setOnClickListener(view -> onLinkClicked());

            control.addView(ellipsizerView);
        }
        if (startIndex != 0) {
            TextView andMoreView  =
                    (TextView) LayoutInflater.from(getContext())
                            .inflate(R.layout.infobar_control_description, control, false);
            String andMoreString = getContext().getResources().getQuantityString(
                    R.plurals.blocked_popups_and_more, startIndex, startIndex);
            andMoreView.setText(andMoreString);
            control.addView(andMoreView);
        }
        layout.setButtons(getContext().getResources().getString(R.string.popups_blocked_infobar_button_always_show), null);
    }

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        String compactString = getContext().getResources().getQuantityString(
                R.plurals.popups_blocked_infobar_text_pl, mBlockedUrls.length,
                mBlockedUrls.length);
        new InfoBarCompactLayout.MessageBuilder(layout)
                .withText(compactString)
                .withLink(R.string.details_link, view -> onLinkClicked())
                .buildAndInsert();
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

    private String getString(@StringRes int stringResId) {
        return getContext().getString(stringResId);
    }

    @CalledByNative
    private static PopupBlockInfoBar create(String[] blockedUrls) {
        return new PopupBlockInfoBar(blockedUrls);
    }
}
