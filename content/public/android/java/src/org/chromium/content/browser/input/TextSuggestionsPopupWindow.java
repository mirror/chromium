// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import android.content.Context;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.TextAppearanceSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.TextView;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.content.R;
import org.chromium.content.browser.WindowAndroidProvider;

public class TextSuggestionsPopupWindow extends SuggestionsPopupWindow {
    private SuggestionInfo[] mSuggestionInfos = new SuggestionInfo[0];
    private TextAppearanceSpan mHighlightSpan;

    /**
     * @param context Android context to use.
     * @param textSuggestionHost TextSuggestionHost instance (used to communicate with Blink).
     * @param parentView The view used to attach the PopupWindow.
     * @param windowAndroidProvider A WindowAndroidProvider instance used to get the window size.
     */
    public TextSuggestionsPopupWindow(Context context, TextSuggestionHost textSuggestionHost,
            View parentView, WindowAndroidProvider windowAndroidProvider) {
        super(context, textSuggestionHost, parentView, windowAndroidProvider);

        mHighlightSpan = new TextAppearanceSpan(mContext, R.style.SuggestionHighlight);
    }

    private class TextSuggestionsAdapter extends BaseAdapter {
        private LayoutInflater mInflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        @Override
        public int getCount() {
            return mNumberOfSuggestionsToUse;
        }

        @Override
        public Object getItem(int position) {
            return mSuggestionInfos[position];
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        @SuppressFBWarnings("BC_UNCONFIRMED_CAST")
        public View getView(int position, View convertView, ViewGroup parent) {
            TextView textView = (TextView) convertView;
            if (textView == null) {
                textView = (TextView) mInflater.inflate(
                        R.layout.text_edit_suggestion_item, parent, false);
            }

            final SuggestionInfo suggestionInfo = mSuggestionInfos[position];
            SpannableString textToSet = new SpannableString(suggestionInfo.getPrefix()
                    + suggestionInfo.getSuggestion() + suggestionInfo.getSuffix());
            textToSet.setSpan(mHighlightSpan, suggestionInfo.getPrefix().length(),
                    suggestionInfo.getPrefix().length() + suggestionInfo.getSuggestion().length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            textView.setText(textToSet);

            return textView;
        }
    }

    protected ListAdapter getSuggestionsAdapter() {
        return new TextSuggestionsAdapter();
    }

    /**
     * Shows the text suggestion menu at the specified coordinates (relative to the viewport).
     */
    public void show(double caretX, double caretY, String highlightedText,
            SuggestionInfo[] suggestionInfos) {
        mSuggestionInfos = suggestionInfos.clone();
        mNumberOfSuggestionsToUse = mSuggestionInfos.length;

        // Android's Editor.java shows the "Add to dictonary" button if and only if there's a
        // SuggestionSpan with FLAG_MISSPELLED set. However, some OEMs (e.g. Samsung) appear to
        // change the behavior on their devices to never show this button, since their IMEs don't go
        // through the normal spell-checking API and instead add SuggestionSpans directly. Since
        // it's difficult to determine how the OEM has set up the native menu, we instead only show
        // the "Add to dictionary" button for spelling markers added by Chrome from running the
        // system spell checker. SuggestionSpans with FLAG_MISSPELLED set (i.e., a spelling
        // underline added directly by the IME) do not show this button.
        mAddToDictionaryButton.setVisibility(View.GONE);
        super.show(caretX, caretY, highlightedText);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        // Ignore taps somewhere in the list footer (divider, "Add to dictionary", "Delete") that
        // don't get handled by a button.
        if (position >= mNumberOfSuggestionsToUse) {
            return;
        }

        SuggestionInfo info = mSuggestionInfos[position];
        mTextSuggestionHost.applyTextSuggestion(info.getMarkerTag(), info.getSuggestionIndex());

        mDismissedByItemTap = true;
        mPopupWindow.dismiss();
    }
}
