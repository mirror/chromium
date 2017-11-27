// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

import android.app.Activity;
import android.app.Fragment;
import android.content.Intent;
import android.os.Bundle;
import android.support.graphics.drawable.VectorDrawableCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar.OnMenuItemClickListener;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.selection.SelectableListLayout;
import org.chromium.chrome.browser.widget.selection.SelectableListToolbar.SearchDelegate;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Fragment with a {@link SelectableListLayout} containing a list of languages.
 */
public class LanguageSelectionFragment
        extends Fragment implements OnMenuItemClickListener, SearchDelegate {
    private static final String EMPTY_QUERY = null;

    private SelectableListLayout mSelectableListLayout;
    private RecyclerView mRecyclerView;
    private TextView mEmptyView;
    private LanguageListAdapter mAdapter;
    private LanguageSelectionToolbar mToolbar;

    /**
     * Listener used to select languages from langauge list.
     */
    interface LanguageSelectionListener {
        /**
         * @param code The language code of the selected item.
         */
        public void onLanguageSelected(String code);
    }

    private static class LanguageListAdapter extends RecyclerView.Adapter<LanguageRowViewHolder> {
        private List<LanguageItem> mFullLanguageList;
        private List<LanguageItem> mLanguageList;
        private LanguageSelectionListener mListener;
        private String mQueryText = EMPTY_QUERY;

        LanguageListAdapter(
                List<LanguageItem> fullLanguageList, LanguageSelectionListener listener) {
            mLanguageList = new ArrayList<LanguageItem>();
            mFullLanguageList = fullLanguageList;
            mListener = listener;
        }

        private void reload(List<LanguageItem> languageList) {
            mLanguageList.clear();
            mLanguageList.addAll(languageList);
            notifyDataSetChanged();
        }

        private void reloadAll() {
            reload(mFullLanguageList);
        }

        @Override
        public LanguageRowViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View languageRow = LayoutInflater.from(parent.getContext())
                                       .inflate(R.layout.accept_languages_item, parent, false);
            return new LanguageRowViewHolder(languageRow);
        }

        @Override
        public void onBindViewHolder(LanguageRowViewHolder holder, int position) {
            LanguageItem info = mLanguageList.get(position);
            holder.setupUI(position, info, mListener);
        }

        @Override
        public int getItemCount() {
            return mLanguageList.size();
        }

        /**
         * Called to perform a search.
         * @param query The text to search for.
         */
        private void search(String query) {
            if (TextUtils.isEmpty(query)) {
                return;
            }
            Locale locale = Locale.getDefault();
            query = query.trim().toLowerCase(locale);
            List<LanguageItem> results = new ArrayList<>();
            for (LanguageItem item : mLanguageList) {
                if (item.getDisplayName().toLowerCase(locale).contains(query)) {
                    results.add(item);
                }
            }
            reload(results);
        }

        /**
         * Called when a search is ended.
         */
        private void onEndSearch() {
            mQueryText = EMPTY_QUERY;
            // mIsSearching = false;

            // Re-initialize the data in the adapter.
            reloadAll();
        }
    }

    private static class LanguageRowViewHolder extends RecyclerView.ViewHolder {
        private View mRow;
        private TextView mTitle;
        private TextView mDesc;

        LanguageRowViewHolder(View view) {
            super(view);

            mRow = view;

            mTitle = (TextView) view.findViewById(R.id.title);
            mDesc = (TextView) view.findViewById(R.id.description);

            view.findViewById(R.id.icon_view).setVisibility(View.GONE);
            view.findViewById(R.id.more).setVisibility(View.GONE);
        }

        private void setupUI(
                final int position, final LanguageItem info, LanguageSelectionListener listener) {
            mTitle.setText(info.getDisplayName());
            mDesc.setText(info.getNativeDisplayName());
            mRow.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    listener.onLanguageSelected(info.getCode());
                }
            });
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ((AppCompatActivity) getActivity()).getSupportActionBar().hide();
        // TODO(crbug/783049): Record User Action.
        // RecordUserAction.record("AddLanguagesPage_impression");
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment.
        View view = inflater.inflate(R.layout.add_languages_main, container, false);

        mSelectableListLayout = (SelectableListLayout) view.findViewById(R.id.selectable_list);

        final Activity activity = getActivity();
        mEmptyView = mSelectableListLayout.initializeEmptyView(
                VectorDrawableCompat.create(getActivity().getResources(), R.drawable.downloads_big,
                        activity.getTheme()),
                R.string.languages_list_no_result, R.string.languages_list_no_result);

        mAdapter = new LanguageListAdapter(
                getLanguagesForSelection(), new LanguageSelectionListener() {
                    @Override
                    public void onLanguageSelected(String code) {
                        Intent intent = new Intent();
                        intent.putExtra(LanguagesPreferences.INTENT_LANGAUGE_ADDED, code);
                        activity.setResult(activity.RESULT_OK, intent);
                        activity.finish();
                    }
                });

        mRecyclerView = mSelectableListLayout.initializeRecyclerView(mAdapter);

        mToolbar = (LanguageSelectionToolbar) mSelectableListLayout.initializeToolbar(
                R.layout.languages_selection_toolbar,
                new SelectionDelegate<LanguageItem>(), // Not support multiple selection now
                R.string.prefs_add_language, null, R.id.normal_menu_group,
                R.id.selection_mode_menu_group,
                FeatureUtilities.isChromeHomeEnabled() ? R.color.modern_toolbar_bg
                                                       : R.color.modern_primary_color,
                this, true);
        mToolbar.initializeSearchView(this, R.string.languages_list_search, R.id.search_menu_id);
        // mToolbar.setInfoMenuItem(R.id.info_menu_id);

        mAdapter.reloadAll();
        mRecyclerView.addItemDecoration(
                new DividerItemDecoration(mRecyclerView.getContext(), LinearLayout.VERTICAL));
        return view;
    }

    @Override
    public boolean onMenuItemClick(MenuItem menuItem) {
        if (menuItem.getItemId() == R.id.close_menu_id) {
            getActivity().finish();
            return true;
        } else if (menuItem.getItemId() == R.id.search_menu_id) {
            mSelectableListLayout.onStartSearch();
            mToolbar.showSearchView();
            return true;
        }

        assert false : "Unhandled menu click.";
        return false;
    }

    @Override
    public void onSearchTextChanged(String query) {
        mAdapter.search(query);
    }

    @Override
    public void onEndSearch() {
        mSelectableListLayout.onEndSearch();
        mAdapter.onEndSearch();
    }

    private List<LanguageItem> getLanguagesForSelection() {
        List<LanguageItem> languagesForSelection = new ArrayList<LanguageItem>();

        // Get user accepted language and all language info from native.
        List<String> preferredLanguages = PrefServiceBridge.getInstance().getUserLanguageCodes();
        List<LanguageItem> languageInfoList =
                PrefServiceBridge.getInstance().getChromeLanguageList();

        // Follow the same order as languageInfoList.
        for (LanguageItem item : languageInfoList) {
            if (!preferredLanguages.contains(item.getCode())) {
                languagesForSelection.add(item);
            }
        }
        return languagesForSelection;
    }
}
