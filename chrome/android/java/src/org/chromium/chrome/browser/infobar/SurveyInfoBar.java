// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

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
 * An {@link InfoBar} that prompts the user to take an optional survey.
 */
public class SurveyInfoBar extends InfoBar {
    // The site to pull a survey from.
    private final String mSiteId;

    // Whether to display the survey as a bottom sheet.
    private final boolean mShowAsBottomSheet;

    // The display logo to be shown on the survey and this infobar.
    private final int mDisplayLogoResId;

    // The observer to handle what happens when info bar events are triggered.
    private SurveyInfoBarObserver mObserver;

    /**
     * Create and show the {@link SurveyInfoBar}.
     * @param webContents The webcontents to create the {@link InfoBar} around.
     * @param siteId The id of the site from where the survey will be downloaded.
     * @param surveyInfoBarObserver The observer to customize what the infobar will do.
     */
    public static void showSurveyInfoBar(WebContents webContents, String siteId,
            boolean showAsBottomSheet, int displayLogoResId,
            SurveyInfoBarObserver surveyInfoBarObserver) {
        nativeCreate(
                webContents, siteId, showAsBottomSheet, displayLogoResId, surveyInfoBarObserver);
    }

    /**
     * Default constructor.
     * @param siteId The id of the site from where the survey will be downloaded.
     * @param showAsBottomSheet Whether the survey should be presented as a bottom sheet or not.
     * @param displayLogoResId Optional resource id of the logo to be displayed on the survey.
     *                         Pass 0 for no logo.
     * @param surveyInfoBarObserver The observer to customize what happens when different events in
     *                              SurveyInfoBar are triggered.
     */
    private SurveyInfoBar(String siteId, boolean showAsBottomSheet, int displayLogoResId,
            SurveyInfoBarObserver surveyInfoBarObserver) {
        super(displayLogoResId, null, null);

        mSiteId = siteId;
        mShowAsBottomSheet = showAsBottomSheet;
        mDisplayLogoResId = displayLogoResId;
        mObserver = surveyInfoBarObserver;
    }

    @Override
    protected boolean usesCompactLayout() {
        return true;
    }

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        NoUnderlineClickableSpan clickableSpan = new NoUnderlineClickableSpan() {
            @Override
            public void onClick(View widget) {
                mObserver.onSurveyAccepted();

                SurveyController.getInstance().showSurveyIfAvailable(
                        nativeGetTab(getNativeInfoBarPtr()).getActivity(), mSiteId,
                        mShowAsBottomSheet, mDisplayLogoResId);
                onCloseButtonClicked();
            }
        };

        TextView prompt = new TextView(getContext());
        prompt.setText(mObserver.getSurveyPromptString(clickableSpan));
        prompt.setMovementMethod(LinkMovementMethod.getInstance());
        prompt.setGravity(Gravity.CENTER_VERTICAL);
        ApiCompatibilityUtils.setTextAppearance(prompt, R.style.BlackTitle1);
        layout.addContent(prompt, 1f);
    }

    @Override
    public void onCloseButtonClicked() {
        mObserver.onSurveyInfoBarClosed();
        super.onCloseButtonClicked();
    }

    @CalledByNative
    private static SurveyInfoBar create(String siteId, boolean showAsBottomSheet,
            int displayLogoResId, SurveyInfoBarObserver surveyInfoBarObserver) {
        return new SurveyInfoBar(
                siteId, showAsBottomSheet, displayLogoResId, surveyInfoBarObserver);
    }

    private static native void nativeCreate(WebContents webContents, String siteId,
            boolean showAsBottomSheet, int displayLogoResId,
            SurveyInfoBarObserver surveyInfoBarObserver);
    private native Tab nativeGetTab(long nativeSurveyInfoBar);
}
