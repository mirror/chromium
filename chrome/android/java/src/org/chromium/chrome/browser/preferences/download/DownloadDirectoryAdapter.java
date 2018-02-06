// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DownloadLocationDialogBridge;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.chrome.browser.widget.TintedImageView;
import org.chromium.chrome.browser.widget.selection.SelectableItemView;

import java.io.File;

/**
 * Custom adapter that populates the list of which directories the user can choose as their default
 * download location.
 */
public class DownloadDirectoryAdapter extends BaseAdapter implements View.OnClickListener {
    private Context mContext;
    private LayoutInflater mLayoutInflater;
    private DownloadDirectoryUtil mDownloadDirectoryUtil;

    private int mSelectedView;
    private boolean mFromDialog;

    DownloadDirectoryAdapter(Context context, boolean fromDialog) {
        mContext = context;
        mLayoutInflater = LayoutInflater.from(mContext);
        mFromDialog = fromDialog;
        mDownloadDirectoryUtil = new DownloadDirectoryUtil(context);
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
        mDownloadDirectoryUtil.clearData();
    }

    private void refreshData() {
        mDownloadDirectoryUtil.refreshData(mContext);
    }

    @Override
    public int getCount() {
        return mDownloadDirectoryUtil.getCount();
    }

    @Override
    public Object getItem(int position) {
        return mDownloadDirectoryUtil.getDirectoryOption(position);
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

        DownloadDirectoryUtil.Option directoryOption =
                (DownloadDirectoryUtil.Option) getItem(position);

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

        Drawable defaultIcon = ((DownloadDirectoryUtil.Option) getItem((int) view.getTag())).mIcon;
        if (FeatureUtilities.isChromeModernDesignEnabled()) {
            // In Modern Design, the default icon is replaced with a check mark if selected.
            SelectableItemView.applyModernIconStyle(startIcon, defaultIcon, isSelected);
            endIcon.setVisibility(View.GONE);
        } else {
            // Otherwise, the selected entry has a blue check mark as the end_icon.
            startIcon.setImageDrawable(defaultIcon);
            endIcon.setVisibility(isSelected ? View.VISIBLE : View.GONE);
        }
    }

    @Override
    public void onClick(View view) {
        int clickedViewPosition = (int) view.getTag();
        DownloadDirectoryUtil.Option directoryOption =
                (DownloadDirectoryUtil.Option) getItem(clickedViewPosition);
        PrefServiceBridge.getInstance().setDownloadAndSaveFileDefaultDirectory(
                directoryOption.mLocation.getAbsolutePath());
        updateSelectedView(view);
        if (mFromDialog) updateAlertDialog(directoryOption.mLocation);
    }

    private void updateSelectedView(View newSelectedView) {
        ListView parentListView = (ListView) newSelectedView.getParent();
        View oldSelectedView = parentListView.getChildAt(mSelectedView);
        styleViewSelectionAndIcons(oldSelectedView, false);

        styleViewSelectionAndIcons(newSelectedView, true);
        mSelectedView = (int) newSelectedView.getTag();
    }

    private void updateAlertDialog(File location) {
        DownloadLocationDialogBridge bridge = DownloadLocationDialogBridge.getInstance();
        if (bridge == null) return;
        bridge.updateFileLocation(location);
    }
}
