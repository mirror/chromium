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

public class SpellCheckPopupWindow extends SuggestionsPopupWindow {
    private String[] mSuggestions = new String[0];
    private TextAppearanceSpan mHighlightSpan;

    /**
     * @param context Android context to use.
     * @param textSuggestionHost TextSuggestionHost instance (used to communicate with Blink).
     * @param parentView The view used to attach the PopupWindow.
     * @param windowAndroidProvider A WindowAndroidProvider instance used to get the window size.
     */
    public SpellCheckPopupWindow(Context context, TextSuggestionHost textSuggestionHost,
            View parentView, WindowAndroidProvider windowAndroidProvider) {
        super(context, textSuggestionHost, parentView, windowAndroidProvider);

        mHighlightSpan = new TextAppearanceSpan(mContext, R.style.SuggestionHighlight);
    }

    private class SpellCheckSuggestionsAdapter extends BaseAdapter {
        private LayoutInflater mInflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        @Override
        public int getCount() {
            return mNumberOfSuggestionsToUse;
        }

        @Override
        public Object getItem(int position) {
            return mSuggestions[position];
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

            SpannableString textToSet = new SpannableString(mSuggestions[position]);
            textToSet.setSpan(
                    mHighlightSpan, 0, textToSet.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            textView.setText(textToSet);

            return textView;
        }
    }

    protected ListAdapter getSuggestionsAdapter() {
        return new SpellCheckSuggestionsAdapter();
    }

    /**
     * Shows the spell check menu at the specified coordinates (relative to the viewport).
     */
    public void show(double caretX, double caretY, String highlightedText, String[] suggestions) {
        mSuggestions = suggestions.clone();
        mNumberOfSuggestionsToUse = mSuggestions.length;

        mAddToDictionaryButton.setVisibility(View.VISIBLE);
        super.show(caretX, caretY, highlightedText);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        // Ignore taps somewhere in the list footer (divider, "Add to dictionary", "Delete") that
        // don't get handled by a button.
        if (position >= mNumberOfSuggestionsToUse) {
            return;
        }

        mTextSuggestionHost.applySpellCheckSuggestion(mSuggestions[position]);

        mDismissedByItemTap = true;
        mPopupWindow.dismiss();
    }
}
