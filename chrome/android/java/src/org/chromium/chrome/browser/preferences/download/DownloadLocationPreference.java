// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.RadioGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

import java.io.File;

/**
 * Preference where users can change download storage location.
 */
public class DownloadLocationPreference extends PreferenceFragment {
    private PrefServiceBridge mPrefServiceBridge;

    private RadioGroup mDownloadLocationGroup;
    private RadioButton mDownloadLocationInternal;
    private RadioButton mDownloadLocationExternal;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPrefServiceBridge = PrefServiceBridge.getInstance();

        getActivity().setTitle(R.string.download_location_title);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.download_location_preference);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        mDownloadLocationGroup = (RadioGroup) view.findViewById(R.id.download_location_group);
        mDownloadLocationInternal =
                (RadioButton) mDownloadLocationGroup.findViewById(R.id.download_location_internal);
        mDownloadLocationExternal =
                (RadioButton) mDownloadLocationGroup.findViewById(R.id.download_location_external);

        showDownloadLocations();
        mDownloadLocationGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int i) {
                updateStorageLocation();
            }
        });

        return view;
    }

    private void showDownloadLocations() {
        // By default, the internal storage location is visible and checked off.
        // If the external storage is writeable, display and potentially check it off.
        if (!isExternalStorageWriteable()) {
            return;
        }

        mDownloadLocationExternal.setVisibility(View.VISIBLE);
        if (Environment.getExternalStorageDirectory().equals(
                    new File(mPrefServiceBridge.getDownloadDefaultDirectory()))) {
            mDownloadLocationExternal.toggle();
        }
    }

    private void updateStorageLocation() {
        // Find the one that is checked and send that information down.
        int checked = mDownloadLocationGroup.getCheckedRadioButtonId();
        String directory;
        switch (checked) {
            case R.id.download_location_external:
                directory = Environment.getExternalStorageDirectory().getAbsolutePath();
                break;
            case R.id.download_location_internal:
            default:
                directory = getDefaultAndroidDownloadPath();
                break;
        }

        mPrefServiceBridge.setDownloadDefaultDirectory(directory);
    }

    private boolean isExternalStorageWriteable() {
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            return true;
        }
        return false;
    }

    private String getDefaultAndroidDownloadPath() {
        return "";
    }
}
