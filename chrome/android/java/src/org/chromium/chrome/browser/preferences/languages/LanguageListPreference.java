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
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListPopupWindow;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.chrome.browser.widget.TintedImageView;

import java.util.ArrayList;
import java.util.List;

/**
 * A preference that displays the current accept language list.
 */
public class LanguageListPreference extends Preference {
    public LanguageListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        RecyclerView listView = (RecyclerView) view.findViewById(R.id.language_list);
        listView.setLayoutManager(new LinearLayoutManager(getContext()));

        // TODO(crbug/783049): Test data only, used for UI testing now.
        ArrayList<String> languageList = new ArrayList<String>();
        PrefServiceBridge.getInstance().getChromeLanguageList(languageList, false);

        LanguageListAdapter languageAdapter = new LanguageListAdapter(languageList,
                new ListPopupWindow(getContext(), null, 0, R.style.BookmarkMenuStyle));
        listView.setAdapter(languageAdapter);
    }

    // TODO(crbug/783049): Move all the classes below out and make the item in the list drag-able.
    private static class LanguageListAdapter extends RecyclerView.Adapter<LanguageItemViewHolder> {
        private List<String> mLanguageList;
        private ListPopupWindow mPopupMenu;

        LanguageListAdapter(List<String> languageList, ListPopupWindow popupMenu) {
            this.mLanguageList = languageList;
            mPopupMenu = popupMenu;
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
            holder.setupUI(position, code, mPopupMenu);
        }

        @Override
        public int getItemCount() {
            return mLanguageList.size();
        }
    }

    private static class LanguageItemViewHolder extends RecyclerView.ViewHolder {
        private Context mContext;
        private TextView mTitel;
        private TintedImageView mStartIcon;
        private TintedImageButton mEndButton;

        LanguageItemViewHolder(View view) {
            super(view);
            mContext = view.getContext();
            mTitel = (TextView) view.findViewById(R.id.title);
            mStartIcon = (TintedImageView) view.findViewById(R.id.icon_view);
            mEndButton = (TintedImageButton) view.findViewById(R.id.more_options);
        }

        private void setupUI(final int langPosition, String title, ListPopupWindow popupMenu) {
            mTitel.setText(title);
            mStartIcon.setImageResource(R.drawable.ic_drag_handle_grey600_24dp);
            mEndButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    showMenu(view, popupMenu);
                }
            });
        }

        private void showMenu(View view, ListPopupWindow popupMenu) {
            // TODO(crbug/783049): Test data only, used for UI testing now.
            String[] testOptionList = new String[] {"Offer to translate", "Remove"};
            popupMenu.setAdapter(new ArrayAdapter<String>(
                    mContext, R.layout.accept_languages_item_popup, testOptionList) {
                private static final int OFFER_TRANSLATE_POSITION = 0;

                @Override
                public boolean areAllItemsEnabled() {
                    return false;
                }

                @Override
                public boolean isEnabled(int position) {
                    if (position == OFFER_TRANSLATE_POSITION) {
                        // TODO(crbug/783049): Return whether Chrome supports to translate to
                        // this language.
                    }
                    return true;
                }

                @Override
                public View getView(int position, View convertView, ViewGroup parent) {
                    View view = super.getView(position, convertView, parent);
                    view.setEnabled(isEnabled(position));

                    TextView textView = (TextView) view;
                    if (position == OFFER_TRANSLATE_POSITION) {
                        // TODO(crbug/783049): Pass whether this language item is selected or not.
                        boolean selected = true;
                        if (selected) {
                            textView.setCompoundDrawablesWithIntrinsicBounds(
                                    0, 0, R.drawable.ic_check_googblue_24dp, 0);
                        }
                        textView.setCompoundDrawablePadding(
                                mContext.getResources().getDimensionPixelSize(
                                        R.dimen.pref_languages_item_popup_drawable_padding));
                    } else {
                        textView.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
                        textView.setCompoundDrawablePadding(0);
                    }

                    return view;
                }
            });
            popupMenu.setAnchorView(view);
            popupMenu.setWidth(mContext.getResources().getDimensionPixelSize(
                    R.dimen.pref_languages_item_popup_width));
            popupMenu.setVerticalOffset(-view.getHeight());
            popupMenu.setModal(true);
            popupMenu.setOnItemClickListener(new OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    // TODO(crbug/783049): Handle click event based on menu name.
                    if (popupMenu != null) popupMenu.dismiss();
                }
            });
            popupMenu.show();
        }
    }
}
