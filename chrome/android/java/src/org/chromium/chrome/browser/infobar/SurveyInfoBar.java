// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.graphics.Typeface;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.survey.SurveyController;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.WebContents;

public class SurveyInfoBar extends InfoBar {
    private static SurveyController sSurveyController;

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
        initSurveyController();
        String surveyInvite = getContext().getResources().getString(R.string.survey_invite);
        String takeSurvey = getContext().getResources().getString(R.string.take_survey);
        SpannableString infoBarText =
                new SpannableString(String.format("%s %s", surveyInvite, takeSurvey));

        ClickableSpan clickableSpan = new ClickableSpan() {

            @Override
            public void onClick(View widget) {
                sSurveyController.showSurvey(nativeGetTab(getNativeInfoBarPtr()).getActivity(),
                        getContext(), SurveyInfoBar.this);
            }

            @Override
            public void updateDrawState(TextPaint drawState) {
                super.updateDrawState(drawState);
                drawState.setUnderlineText(false);
            }
        };

        infoBarText.setSpan(clickableSpan, infoBarText.length() - takeSurvey.length(),
                infoBarText.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

        TextView prompt = new TextView(getContext());
        prompt.setText(infoBarText);
        prompt.setTextSize(TypedValue.COMPLEX_UNIT_PX,
                getContext().getResources().getDimension(R.dimen.infobar_text_size));
        prompt.setTextColor(
                ApiCompatibilityUtils.getColor(layout.getResources(), R.color.infobar_text_color));
        prompt.setTypeface(Typeface.SANS_SERIF);
        prompt.setGravity(Gravity.CENTER_VERTICAL);
        prompt.setMovementMethod(LinkMovementMethod.getInstance());
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

    private void initSurveyController() {
        sSurveyController = SurveyController.getInstance();
        sSurveyController.setSurveyId(mSurveyId);
        sSurveyController.setAdvertisingId(mAdvertisingId == null ? "" : mAdvertisingId);
        sSurveyController.registerReceiver(getContext());
    }

    private static native void nativeCreate(
            WebContents webContents, String surveyId, String advertisingId);
    private native Tab nativeGetTab(long nativeSurveyInfoBar);
}