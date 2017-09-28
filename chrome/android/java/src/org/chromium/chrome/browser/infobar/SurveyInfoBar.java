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
    private String mSurveyId;
    private String mAdvertisingId;

    /**
     * Default constructor.
     */
    private SurveyInfoBar(String surveyId, String advertisingId) {
        super(R.drawable.chrome_sync_logo, null, null);
        mSurveyId = surveyId;
        mAdvertisingId = advertisingId;
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
                        nativeGetTab(getNativeInfoBarPtr()).getActivity());
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
    private static SurveyInfoBar create(String surveyId, String advertisingId) {
        return new SurveyInfoBar(surveyId, advertisingId);
    }

    /**
     * Create and show the {@link SurveyInfoBar}.
     * @param webContents The webcontents to create the {@link InfoBar} around.
     * @param surveyId The id of the survey to show.
     * @param advertisingId The advertising id used to build the survey request.
     */
    public static void showSurveyInfoBar(
            WebContents webContents, String surveyId, String advertisingId) {
        nativeCreate(webContents, surveyId, advertisingId);
    }

    private static native void nativeCreate(
            WebContents webContents, String surveyId, String advertisingId);
    private native Tab nativeGetTab(long nativeSurveyInfoBar);
}