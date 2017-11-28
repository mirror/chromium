// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

import android.content.Context;
import android.preference.Preference;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.widget.ListMenuButton;
import org.chromium.chrome.browser.widget.ListMenuButton.Item;
import org.chromium.chrome.browser.widget.TintedDrawable;
import org.chromium.chrome.browser.widget.TintedImageView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A preference that displays the current accept language list.
 */
public class LanguageListPreference extends Preference {
    /**
     * Used to update languages list.
     */
    interface LanguageListUpdater {
        /**
         * @param code The language code to add.
         */
        void addLanguage(String code);

        /**
         * @param code The language code to remove.
         */
        void removeLanguage(String code);
    }

    private View mView;
    private Button mAddLanguage;
    private RecyclerView mRecyclerView;
    private LanguageListAdapter mAdapter;

    private OnClickListener mAddButtonClickListener;

    public LanguageListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mAdapter = new LanguageListAdapter();
    }

    @Override
    public View onCreateView(ViewGroup parent) {
        if (mView != null) return mView;

        mView = super.onCreateView(parent);

        mAddLanguage = (Button) mView.findViewById(R.id.add_language);
        ApiCompatibilityUtils.setCompoundDrawablesRelativeWithIntrinsicBounds(mAddLanguage,
                TintedDrawable.constructTintedDrawable(
                        getContext().getResources(), R.drawable.plus, R.color.pref_accent_color),
                null, null, null);
        mAddLanguage.setOnClickListener(mAddButtonClickListener);

        mRecyclerView = (RecyclerView) mView.findViewById(R.id.language_list);
        LinearLayoutManager layoutMangager = new LinearLayoutManager(getContext());
        mRecyclerView.setLayoutManager(layoutMangager);
        mRecyclerView.addItemDecoration(
                new DividerItemDecoration(getContext(), layoutMangager.getOrientation()));

        return mView;
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        // We do not want the RecyclerView to be announced every time the view is bound.
        if (mRecyclerView.getAdapter() != mAdapter) {
            mRecyclerView.setAdapter(mAdapter);
            // Initialize language list.
            mAdapter.reload();
        }
    }

    void setAddButtonClickListener(OnClickListener listener) {
        mAddButtonClickListener = listener;
    }

    void addLanguageFromSelection(String code) {
        if (mAdapter == null) return;
        ((LanguageListUpdater) mAdapter).addLanguage(code);
    }

    // TODO(crbug/783049): Pull all the inner classes below out and make the item in the list
    // drag-able.
    private static class LanguageListAdapter
            extends RecyclerView.Adapter<LanguageItemViewHolder> implements LanguageListUpdater {
        private final Map<String, LanguageItem> mLanguagesMap;
        private List<LanguageItem> mLanguageList;
        private PrefServiceBridge mPrefServiceBridge;

        LanguageListAdapter() {
            mPrefServiceBridge = PrefServiceBridge.getInstance();
            // Get all language info from native.
            mLanguagesMap = new HashMap<>();
            List<LanguageItem> languageInfos = mPrefServiceBridge.getChromeLanguageList();
            for (LanguageItem item : languageInfos) {
                mLanguagesMap.put(item.getCode(), item);
            }

            mLanguageList = new ArrayList<>();
        }

        @Override
        public LanguageItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View languageRow = LayoutInflater.from(parent.getContext())
                                       .inflate(R.layout.accept_languages_item, parent, false);
            return new LanguageItemViewHolder(languageRow, this);
        }

        @Override
        public void onBindViewHolder(LanguageItemViewHolder holder, int position) {
            LanguageItem info = mLanguageList.get(position);
            holder.setupUI(getItemCount(), info);
        }

        @Override
        public int getItemCount() {
            return mLanguageList.size();
        }

        @Override
        public void addLanguage(String code) {
            mPrefServiceBridge.updateUserAcceptLanguages(code, true);
            reload();
        }

        @Override
        public void removeLanguage(String code) {
            mPrefServiceBridge.updateUserAcceptLanguages(code, false);
            reload();
        }

        private void reload() {
            // Always get the latest user preferred language list from native.
            List<String> codes = mPrefServiceBridge.getUserLanguageCodes();

            mLanguageList.clear();
            // Keep the same order as codes.
            for (String code : codes) {
                LanguageItem item = mLanguagesMap.get(code);
                if (item == null) continue;
                mLanguageList.add(item);
            }

            notifyDataSetChanged();
        }
    }

    private static class LanguageItemViewHolder extends RecyclerView.ViewHolder {
        private static final int OFFER_TRANSLATE_POSITION = 0;

        private Context mContext;
        private View mRow;
        private TextView mTitle;
        private TextView mDescription;
        private TintedImageView mStartIcon;
        private ListMenuButton mMoreButton;

        private LanguageListUpdater mUpdater;

        LanguageItemViewHolder(View view, LanguageListUpdater updater) {
            super(view);
            mContext = view.getContext();
            mRow = view;
            mTitle = (TextView) view.findViewById(R.id.title);
            mDescription = (TextView) view.findViewById(R.id.description);
            mStartIcon = (TintedImageView) view.findViewById(R.id.icon_view);
            mMoreButton = (ListMenuButton) view.findViewById(R.id.more);
            mUpdater = updater;
        }

        private void setupUI(final int count, final LanguageItem info) {
            mStartIcon.setImageResource(R.drawable.ic_drag_handle_grey600_24dp);
            mTitle.setText(info.getDisplayName());
            if (TextUtils.equals(info.getDisplayName(), info.getNativeDisplayName())) {
                mDescription.setVisibility(View.GONE);
            } else {
                mDescription.setVisibility(View.VISIBLE);
                mDescription.setText(info.getNativeDisplayName());
            }

            mMoreButton.setDelegate(new ListMenuButton.Delegate() {
                @Override
                public Item[] getItems() {
                    return new Item[] {
                            new Item(mContext, R.string.languages_item_option_offer_to_translate,
                                    R.drawable.ic_check_googblue_24dp, info.isSupported()),
                            new Item(mContext, R.string.remove,
                                    count > 1 /* Can't remove the last item */)};
                }

                @Override
                public void onItemSelected(Item item) {
                    if (item.getTextId() == R.string.languages_item_option_offer_to_translate) {
                        // TODO(crbug/783049): Handle "offer to translate" event.
                    } else if (item.getTextId() == R.string.remove) {
                        mUpdater.removeLanguage(info.getCode());
                    }
                }
            });
        }
    }
}
