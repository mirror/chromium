// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

import android.content.Context;
import android.preference.Preference;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;

import java.util.ArrayList;
import java.util.List;

/**
 * A preference that displays the current accept language list.
 */
public class LanguageListPreference extends Preference {
    private final LanguageListAdapter mLanguageAdapter;

    public LanguageListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        // TODO(crbug/783049): Test data only, used for UI testing now.
        ArrayList<String> languageList = new ArrayList<String>();
        PrefServiceBridge.getInstance().getChromeLanguageList(languageList, false);

        mLanguageAdapter = new LanguageListAdapter(languageList);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        RecyclerView listView = (RecyclerView) view.findViewById(R.id.language_list);

        listView.setLayoutManager(new LinearLayoutManager(getContext()));
        listView.setAdapter(mLanguageAdapter);

        // TODO(crbug/783049): Implement "Add Language" button click callback, which will
        // show all langauges in another page for selection.
    }

    // TODO(crbug/783049): Move all the classes below out and make the item in the list drag-able.
    private static class LanguageListAdapter extends RecyclerView.Adapter<LanguageItemViewHolder> {
        private List<String> mLanguageList;

        LanguageListAdapter(List<String> languageList) {
            this.mLanguageList = languageList;
        }

        @Override
        public LanguageItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View languageItem = LayoutInflater.from(parent.getContext())
                                        .inflate(R.layout.accept_languages_item, parent, false);
            return new LanguageItemViewHolder(languageItem);
        }

        @Override
        public void onBindViewHolder(LanguageItemViewHolder holder, int position) {
            String code = mLanguageList.get(position);
            // TODO(crbug/783049): Get language name by code and Locale from LocalizationUtils.
            // displayLocale = context.getResources().getConfiguration().locale;
            holder.mNameText.setText(code);
            // Set options for each language item.
            holder.initOptions(position);
        }

        @Override
        public int getItemCount() {
            return mLanguageList.size();
        }
    }

    private static class LanguageItemViewHolder extends RecyclerView.ViewHolder {
        private TextView mNameText;
        private Spinner mOptions;

        LanguageItemViewHolder(View view) {
            super(view);
            mNameText = (TextView) view.findViewById(R.id.language_name);
            mOptions = (Spinner) view.findViewById(R.id.language_options);
        }

        private void initOptions(final int langPosition) {
            // TODO(crbug/783049): Test data only, used for UI testing now.
            String[] testOptionList = new String[] {"Offer to translate", "Remove"};

            LanguageOptionsAdapter adapter =
                    new LanguageOptionsAdapter(mOptions.getContext(), testOptionList);
            adapter.setDropDownViewResource(R.layout.accept_languages_options_dropdown);
            mOptions.setAdapter(adapter);
            mOptions.setOnItemSelectedListener(new OnItemSelectedListener() {
                @Override
                public void onItemSelected(
                        AdapterView<?> parent, View view, int position, long id) {
                    // TODO(crbug/783049): Apply dropdown options for each item.
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                    // Nothing.
                }
            });
        }
    }

    private static class LanguageOptionsAdapter extends ArrayAdapter<String> {
        private final Context mContext;
        private final String[] mDropDownList;

        LanguageOptionsAdapter(Context context, String[] dropDownList) {
            super(context, R.layout.accept_languages_options, dropDownList);
            this.mContext = context;
            this.mDropDownList = dropDownList;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = LayoutInflater.from(mContext).inflate(
                        R.layout.accept_languages_options, parent, false);
            }
            return convertView;
        }

        @Override
        public View getDropDownView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = LayoutInflater.from(mContext).inflate(
                        R.layout.accept_languages_options_dropdown, parent, false);
            }
            TextView t = (TextView) convertView;
            t.setText(mDropDownList[position]);
            return convertView;
        }

        private void setChecked(TextView textView, boolean checked) {
            if (checked) {
                textView.setCompoundDrawablesWithIntrinsicBounds(
                        0, 0, R.drawable.bookmark_check_gray, 0);
                textView.setCompoundDrawablePadding(mContext.getResources().getDimensionPixelSize(
                        R.dimen.pref_languages_spacing_small));
            } else {
                textView.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
                textView.setCompoundDrawablePadding(0);
            }
        }
    }
}
