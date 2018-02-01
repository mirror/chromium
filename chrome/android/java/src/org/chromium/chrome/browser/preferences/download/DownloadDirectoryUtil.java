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

import org.chromium.chrome.browser.download.ui.DownloadFilter;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A utility class that helps maintain the available directories for downloading.
 */
public class DownloadDirectoryUtil {
    class Option {
        Option(String directoryName, Drawable directoryIcon, File directoryLocation,
                String availableSpace) {
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

    private List<Pair<String, Integer>> mCanonicalDirectoryPairs = new ArrayList<>();
    private List<Option> mCanonicalDirectoryOptions = new ArrayList<>();
    private List<Option> mAdditionalDirectoryOptions = new ArrayList<>();
    private List<List<Option>> mAllDirectoryOptions =
            Arrays.asList(mCanonicalDirectoryOptions, mAdditionalDirectoryOptions);

    /**
     * Create a DownloadDirectoryUtil based on a given context.
     *
     * @param context   The context in which the DownloadDirectoryUtil exists.
     */
    public DownloadDirectoryUtil(Context context) {
        // Build canonical directory pairs.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            mCanonicalDirectoryPairs.add(
                    Pair.create(Environment.DIRECTORY_DOCUMENTS, DownloadFilter.FILTER_DOCUMENT));
        }

        mCanonicalDirectoryPairs.add(
                Pair.create(Environment.DIRECTORY_PICTURES, DownloadFilter.FILTER_IMAGE));
        mCanonicalDirectoryPairs.add(
                Pair.create(Environment.DIRECTORY_MUSIC, DownloadFilter.FILTER_AUDIO));
        mCanonicalDirectoryPairs.add(
                Pair.create(Environment.DIRECTORY_MOVIES, DownloadFilter.FILTER_VIDEO));
        mCanonicalDirectoryPairs.add(
                Pair.create(Environment.DIRECTORY_DOWNLOADS, DownloadFilter.FILTER_ALL));

        refreshData(context);
    }

    /**
     * Refresh the information that is available in the DownloadDirectoryUtil.
     *
     * @param context   The context in which the DownloadDirectoryUtil exists.
     */
    void refreshData(Context context) {
        mContext = context;

        setCanonicalDirectoryOptions();
        setAdditionalDirectoryOptions();
    }

    /**
     * Clear the information that is kept by the DownloadDirectoryUtil.
     */
    void clearData() {
        mCanonicalDirectoryOptions.clear();
        mAdditionalDirectoryOptions.clear();
    }

    /**
     * @return  The number of directory options there are, including canonical and additional.
     */
    int getCount() {
        return mCanonicalDirectoryOptions.size() + mAdditionalDirectoryOptions.size();
    }

    /**
     * Get a specific directory option for a position.
     *
     * @param position  The index of the directory option that is to be returned.
     * @return          The directory option for that given position.
     */
    Option getDirectoryOption(int position) {
        int canonicalDirectoriesEndPosition = mCanonicalDirectoryOptions.size() - 1;
        if (position <= canonicalDirectoriesEndPosition) {
            return mCanonicalDirectoryOptions.get(position);
        } else {
            return mAdditionalDirectoryOptions.get(position - canonicalDirectoriesEndPosition - 1);
        }
    }

    /**
     * Get a file location given the display name of the file.
     *
     * @param name  The display name of the file.
     * @return      The actual location of the file with that given display name.
     */
    public File getFileForName(String name) {
        for (List<Option> optionList : mAllDirectoryOptions) {
            for (Option option : optionList) {
                if (option.mName.equals(name)) {
                    return option.mLocation;
                }
            }
        }
        return Environment.getExternalStorageDirectory();
    }

    /**
     * Get a display name for a given file location.
     *
     * @param file  The file location.
     * @return      The display name associated with that file location.
     */
    public String getNameForFile(File file) {
        for (List<Option> optionList : mAllDirectoryOptions) {
            for (Option option : optionList) {
                if (option.mLocation.equals(file)) {
                    return option.mName;
                }
            }
        }
        return "";
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
            mCanonicalDirectoryOptions.add(
                    new Option(directoryName, directoryIcon, directoryLocation, availableBytes));
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

            int numOtherAdditionalDirectories = mAdditionalDirectoryOptions.size();
            // Add index (ie. SD Card 2) if there is more than one secondary storage option.
            String directoryName = (numOtherAdditionalDirectories > 0)
                    ? mContext.getString(
                              org.chromium.chrome.R.string.downloads_location_sd_card_number,
                              numOtherAdditionalDirectories + 1)
                    : mContext.getString(org.chromium.chrome.R.string.downloads_location_sd_card);

            Drawable directoryIcon = VectorDrawableCompat.create(mContext.getResources(),
                    org.chromium.chrome.R.drawable.ic_sd_storage, mContext.getTheme());
            String availableBytes = getAvailableBytesString(dir);

            mAdditionalDirectoryOptions.add(
                    new Option(directoryName, directoryIcon, dir, availableBytes));
        }
    }

    private String getAvailableBytesString(File file) {
        return Formatter.formatFileSize(mContext, file.getUsableSpace());
    }
}
