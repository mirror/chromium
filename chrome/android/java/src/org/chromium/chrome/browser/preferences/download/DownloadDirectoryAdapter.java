// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.os.Environment;
import android.support.v4.content.ContextCompat;
import android.text.format.Formatter;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.ui.DownloadFilter;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Custom adapter that populates the list of which directories the user can choose as their default
 * download location.
 */
public class DownloadDirectoryAdapter extends BaseAdapter implements View.OnClickListener {
    private static class DownloadDirectoryOption {
        DownloadDirectoryOption(String directoryName, int directoryIcon, File directoryLocation,
                String availableSpace) {
            this.mName = directoryName;
            this.mIcon = directoryIcon;
            this.mLocation = directoryLocation;
            this.mAvailableSpace = availableSpace;
        }

        String mName;
        int mIcon;
        File mLocation;
        String mAvailableSpace;
    }

    private Context mContext;
    private LayoutInflater mLayoutInflater;

    private List<DownloadDirectoryOption> mCanonicalDirectoryOptions = new ArrayList<>();
    private List<DownloadDirectoryOption> mAdditionalDirectoryOptions = new ArrayList<>();
    private List<Pair<String, Integer>> mCanonicalDirectoryPairs = new ArrayList<>();

    public DownloadDirectoryAdapter(Context context) {
        mContext = context;
        mLayoutInflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        if (Build.VERSION.SDK_INT >= 19) {
            mCanonicalDirectoryPairs.add(new Pair<>(Environment.DIRECTORY_DOCUMENTS, 5));
        }

        mCanonicalDirectoryPairs.add(new Pair<>(Environment.DIRECTORY_PICTURES, 4));
        mCanonicalDirectoryPairs.add(new Pair<>(Environment.DIRECTORY_MUSIC, 3));
        mCanonicalDirectoryPairs.add(new Pair<>(Environment.DIRECTORY_MOVIES, 2));
        mCanonicalDirectoryPairs.add(new Pair<>(Environment.DIRECTORY_DOWNLOADS, 0));
    }

    public void start() {
        refreshData();
    }

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
            int directoryIcon = DownloadFilter.getDrawableForFilter(nameAndIndex.second);

            File directoryLocation =
                    Environment.getExternalStoragePublicDirectory(nameAndIndex.first);
            String availableBytes = getAvailableBytesString(directoryLocation);
            mCanonicalDirectoryOptions.add(new DownloadDirectoryOption(
                    directoryName, directoryIcon, directoryLocation, availableBytes));
        }
    }

    private void setAdditionalDirectoryOptions() {
        // TODO(jming): Is there any way to do this for API < 19????
        if (Build.VERSION.SDK_INT >= 19) {
            File[] externalDirs = mContext.getExternalFilesDirs(Environment.DIRECTORY_DOWNLOADS);
            int numAdditionalDirectories = externalDirs.length - 1;
            if (numAdditionalDirectories != mAdditionalDirectoryOptions.size()) {
                mAdditionalDirectoryOptions.clear();
                if (numAdditionalDirectories == 0) return;
                for (File dir : externalDirs) {
                    if (dir == null) continue;

                    // Skip the directory in primary storage.
                    if (dir.getAbsolutePath().contains(
                                Environment.getExternalStorageDirectory().getAbsolutePath())) {
                        continue;
                    }

                    String directoryName = mContext.getString(R.string.downloads_location_sd_card);
                    int numOtherAdditionalDirectories = mAdditionalDirectoryOptions.size();
                    // Add index (ie. SD Card 2) if there is more than one secondary storage option.
                    if (numOtherAdditionalDirectories > 0) {
                        directoryName =
                                directoryName + " " + String.valueOf(numOtherAdditionalDirectories);
                    }

                    int directoryIcon = android.R.drawable.stat_notify_sdcard;
                    String availableBytes = getAvailableBytesString(dir);

                    mAdditionalDirectoryOptions.add(new DownloadDirectoryOption(
                            directoryName, directoryIcon, dir, availableBytes));
                }
            }
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
        int numCanonicalDirectories = mCanonicalDirectoryOptions.size() - 1;
        if (position <= numCanonicalDirectories) {
            return mCanonicalDirectoryOptions.get(position);
        } else {
            return mAdditionalDirectoryOptions.get(numCanonicalDirectories - position);
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

        ImageView directoryIcon = (ImageView) view.findViewById(R.id.directory_icon);
        directoryIcon.setImageResource(directoryOption.mIcon);
        int padding = mContext.getResources().getDimensionPixelSize(R.dimen.pref_icon_padding);
        ApiCompatibilityUtils.setPaddingRelative(directoryIcon, padding,
                directoryIcon.getPaddingTop(), 0, directoryIcon.getPaddingBottom());

        TextView directoryName = (TextView) view.findViewById(R.id.directory_name);
        directoryName.setText(directoryOption.mName);

        TextView spaceAvailable = (TextView) view.findViewById(R.id.available_space);
        spaceAvailable.setText(directoryOption.mAvailableSpace);

        if (directoryOption.mLocation.getAbsolutePath().equals(
                    PrefServiceBridge.getInstance().getDownloadDefaultDirectory())) {
            view.setSelected(true);
            styleViewSelection(view, true);
        } else {
            view.setSelected(false);
            styleViewSelection(view, false);
        }

        return view;
    }

    private void styleViewSelection(View view, boolean isSelected) {
        if (isSelected) {
            view.setBackgroundColor(ContextCompat.getColor(mContext, R.color.google_blue_300));
        } else {
            view.setBackgroundColor(Color.TRANSPARENT);
        }
    }

    @Override
    public void onClick(View view) {
        DownloadDirectoryOption directoryOption =
                (DownloadDirectoryOption) getItem((int) view.getTag());
        PrefServiceBridge.getInstance().setDownloadAndSaveFileDefaultDirectory(
                directoryOption.mLocation.getAbsolutePath());
        notifyDataSetChanged();
    }
}
