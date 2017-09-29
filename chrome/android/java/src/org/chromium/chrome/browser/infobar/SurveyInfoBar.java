// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.survey.SurveyController;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * An info bar that prompts the user to take an optional survey.
 */
public class SurveyInfoBar extends InfoBar {
    private final String mSiteId;
    private final boolean mShowAsBottomSheet;
    private final int mDisplayLogo;

    /**
     * Create and show the {@link SurveyInfoBar}.
     * @param webContents The webcontents to create the {@link InfoBar} around.
     * @param siteId The id of the site from where the survey will be downloaded.
     */
    public static void showSurveyInfoBar(
            WebContents webContents, String siteId, boolean showAsBottomSheet, int displayLogo) {
        nativeCreate(webContents, siteId, showAsBottomSheet, displayLogo);
    }

    /**
     * Default constructor.
     * @param siteId The id of the site from where the survey will be downloaded.
     * @param showAsBottomSheet Whether the survey should be presented as a bottom sheet or not.
     * @param displayLogo Optional resource id of the logo to be displayed on the survey. Pass 0 for
     *                    no logo.
     */
    private SurveyInfoBar(String siteId, boolean showAsBottomSheet, int displayLogo) {
        super(R.drawable.chrome_sync_logo, null, null);

        mSiteId = siteId;
        mShowAsBottomSheet = showAsBottomSheet;
        mDisplayLogo = displayLogo;
    }

    @Override
    protected boolean usesCompactLayout() {
        return true;
    }

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        String surveyInvite = getContext().getResources().getString(R.string.survey_invite);
        String takeSurvey = getContext().getResources().getString(R.string.take_survey);
        SpannableString infoBarText =
                new SpannableString(String.format("%s %s", surveyInvite, takeSurvey));

        NoUnderlineClickableSpan clickableSpan = new NoUnderlineClickableSpan() {

            @Override
            public void onClick(View widget) {
                SurveyController.getInstance().showSurveyIfAvailable(
                        nativeGetTab(getNativeInfoBarPtr()).getActivity(), mSiteId,
                        mShowAsBottomSheet, mDisplayLogo);
                onCloseButtonClicked();
            }
        };

        infoBarText.setSpan(clickableSpan, infoBarText.length() - takeSurvey.length(),
                infoBarText.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

        TextView prompt = new TextView(getContext());
        prompt.setText(infoBarText);
        prompt.setMovementMethod(LinkMovementMethod.getInstance());
        prompt.setGravity(Gravity.CENTER_VERTICAL);
        ApiCompatibilityUtils.setTextAppearance(prompt, R.style.BlackTitle1);
        layout.addContent(prompt, 1f);
    }

    @CalledByNative
    private static SurveyInfoBar create(String siteId, boolean showAsBottomSheet, int displayLogo) {
        return new SurveyInfoBar(siteId, showAsBottomSheet, displayLogo);
    }

    private static native void nativeCreate(
            WebContents webContents, String siteId, boolean showAsBottomSheet, int displayLogo);
    private native Tab nativeGetTab(long nativeSurveyInfoBar);
}