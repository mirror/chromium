// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Environment;
import android.support.graphics.drawable.VectorDrawableCompat;
import android.support.v4.content.ContextCompat;
import android.text.format.Formatter;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.ui.DownloadFilter;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.TintedDrawable;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.chrome.browser.widget.TintedImageView;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Custom adapter that populates the list of which directories the user can choose as their default
 * download location.
 */
public class DownloadDirectoryAdapter extends BaseAdapter implements View.OnClickListener {
    private static class DownloadDirectoryOption {
        DownloadDirectoryOption(String directoryName, Drawable directoryIcon,
                File directoryLocation, String availableSpace) {
            this.mName = directoryName;
            this.mIcon = directoryIcon;
            this.mLocation = directoryLocation;
            this.mAvailableSpace = availableSpace;
        }

        String mName;
        Drawable mIcon;
        File mLocation;
        String mAvailableSpace;
    }

    private Context mContext;
    private LayoutInflater mLayoutInflater;

    private List<Pair<String, Integer>> mCanonicalDirectoryPairs = new ArrayList<>();
    private List<DownloadDirectoryOption> mCanonicalDirectoryOptions = new ArrayList<>();
    private List<DownloadDirectoryOption> mAdditionalDirectoryOptions = new ArrayList<>();

    private int mSelectedView;

    public DownloadDirectoryAdapter(Context context) {
        mContext = context;
        mLayoutInflater = LayoutInflater.from(mContext);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            mCanonicalDirectoryPairs.add(
                    new Pair<>(Environment.DIRECTORY_DOCUMENTS, DownloadFilter.FILTER_DOCUMENT));
        }

        mCanonicalDirectoryPairs.add(
                new Pair<>(Environment.DIRECTORY_PICTURES, DownloadFilter.FILTER_IMAGE));
        mCanonicalDirectoryPairs.add(
                new Pair<>(Environment.DIRECTORY_MUSIC, DownloadFilter.FILTER_AUDIO));
        mCanonicalDirectoryPairs.add(
                new Pair<>(Environment.DIRECTORY_MOVIES, DownloadFilter.FILTER_VIDEO));
        mCanonicalDirectoryPairs.add(
                new Pair<>(Environment.DIRECTORY_DOWNLOADS, DownloadFilter.FILTER_ALL));
    }

    /**
     * Start the adapter to gather the available directories.
     */
    public void start() {
        refreshData();
    }

    /**
     * Stop the adapter and reset lists of directories.
     */
    public void stop() {
        mCanonicalDirectoryOptions.clear();
        mAdditionalDirectoryOptions.clear();
    }

    private void setCanonicalDirectoryOptions() {
        if (mCanonicalDirectoryOptions.size() == mCanonicalDirectoryPairs.size()) return;
        mCanonicalDirectoryOptions.clear();

        for (Pair<String, Integer> nameAndIndex : mCanonicalDirectoryPairs) {
            String directoryName =
                    mContext.getString(DownloadFilter.getStringIdForFilter(nameAndIndex.second));
            Drawable directoryIcon = ContextCompat.getDrawable(
                    mContext, DownloadFilter.getDrawableForFilter(nameAndIndex.second));

            File directoryLocation =
                    Environment.getExternalStoragePublicDirectory(nameAndIndex.first);
            String availableBytes = getAvailableBytesString(directoryLocation);
            mCanonicalDirectoryOptions.add(new DownloadDirectoryOption(
                    directoryName, directoryIcon, directoryLocation, availableBytes));
        }
    }

    private void setAdditionalDirectoryOptions() {
        mAdditionalDirectoryOptions.clear();

        // TODO(jming): Is there any way to do this for API < 19????
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) return;

        File[] externalDirs = mContext.getExternalFilesDirs(Environment.DIRECTORY_DOWNLOADS);
        int numAdditionalDirectories = externalDirs.length - 1;

        // If there are no more additional directories, it is only the primary storage available.
        if (numAdditionalDirectories == 0) return;

        for (File dir : externalDirs) {
            if (dir == null) continue;

            // Skip the directory that is in primary storage.
            if (dir.getAbsolutePath().contains(
                        Environment.getExternalStorageDirectory().getAbsolutePath())) {
                continue;
            }

            String directoryName = mContext.getString(R.string.downloads_location_sd_card);
            int numOtherAdditionalDirectories = mAdditionalDirectoryOptions.size();
            // Add index (ie. SD Card 2) if there is more than one secondary storage option.
            if (numOtherAdditionalDirectories > 0) {
                directoryName =
                        directoryName + " " + String.valueOf(numOtherAdditionalDirectories + 1);
            }

            Drawable directoryIcon = VectorDrawableCompat.create(
                    mContext.getResources(), R.drawable.ic_sd_storage, mContext.getTheme());
            String availableBytes = getAvailableBytesString(dir);

            mAdditionalDirectoryOptions.add(
                    new DownloadDirectoryOption(directoryName, directoryIcon, dir, availableBytes));
        }
    }

    private String getAvailableBytesString(File file) {
        return Formatter.formatFileSize(mContext, file.getUsableSpace());
    }

    private void refreshData() {
        setCanonicalDirectoryOptions();
        setAdditionalDirectoryOptions();
    }

    @Override
    public int getCount() {
        return mCanonicalDirectoryOptions.size() + mAdditionalDirectoryOptions.size();
    }

    @Override
    public Object getItem(int position) {
        int canonicalDirectoriesEndPosition = mCanonicalDirectoryOptions.size() - 1;
        if (position <= canonicalDirectoriesEndPosition) {
            return mCanonicalDirectoryOptions.get(position);
        } else {
            return mAdditionalDirectoryOptions.get(position - canonicalDirectoriesEndPosition - 1);
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup viewGroup) {
        View view = convertView;
        if (view == null) {
            view = mLayoutInflater.inflate(R.xml.download_directory, null);
        }

        view.setClickable(true);
        view.setOnClickListener(this);
        view.setTag(position);

        DownloadDirectoryOption directoryOption = (DownloadDirectoryOption) getItem(position);

        TextView directoryName = (TextView) view.findViewById(R.id.title);
        directoryName.setText(directoryOption.mName);

        TextView spaceAvailable = (TextView) view.findViewById(R.id.description);
        spaceAvailable.setText(directoryOption.mAvailableSpace);

        if (directoryOption.mLocation.getAbsolutePath().equals(
                    PrefServiceBridge.getInstance().getDownloadDefaultDirectory())) {
            styleViewSelectionAndIcons(view, true);
            mSelectedView = position;
        } else {
            styleViewSelectionAndIcons(view, false);
        }

        return view;
    }

    private void styleViewSelectionAndIcons(View view, boolean isSelected) {
        TintedImageView startIcon = view.findViewById(R.id.icon_view);
        TintedImageButton endIcon = view.findViewById(R.id.selected_view);

        Drawable directoryIcon = ((DownloadDirectoryOption) getItem((int) view.getTag())).mIcon;
        if (FeatureUtilities.isChromeModernDesignEnabled()) {
            startIcon.setBackgroundResource(R.drawable.list_item_icon_modern_bg);
            startIcon.setImageDrawable(isSelected
                            ? TintedDrawable.constructTintedDrawable(view.getResources(),
                                      R.drawable.ic_check_googblue_24dp, R.color.white_mode_tint)
                            : directoryIcon);
            startIcon.getBackground().setLevel(isSelected
                            ? view.getResources().getInteger(R.integer.list_item_level_selected)
                            : view.getResources().getInteger(R.integer.list_item_level_default));
            endIcon.setVisibility(View.GONE);
        } else {
            // Selected entry has an end_icon, a blue check mark.
            startIcon.setImageDrawable(directoryIcon);
            endIcon.setVisibility(isSelected ? View.VISIBLE : View.GONE);
        }
    }

    @Override
    public void onClick(View view) {
        int clickedViewPosition = (int) view.getTag();
        DownloadDirectoryOption directoryOption =
                (DownloadDirectoryOption) getItem(clickedViewPosition);
        PrefServiceBridge.getInstance().setDownloadAndSaveFileDefaultDirectory(
                directoryOption.mLocation.getAbsolutePath());
        updateSelectedView(view);
    }

    private void updateSelectedView(View newSelectedView) {
        ListView parentListView = (ListView) newSelectedView.getParent();
        View oldSelectedView = parentListView.getChildAt(mSelectedView);
        styleViewSelectionAndIcons(oldSelectedView, false);

        styleViewSelectionAndIcons(newSelectedView, true);
        mSelectedView = (int) newSelectedView.getTag();
    }
}
