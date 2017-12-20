// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.os.Environment;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;

import org.chromium.chrome.R;

import java.io.File;

/**
 * Fragment to keep track of all downloads related preferences.
 */
public class DownloadPreferences
        extends PreferenceFragment implements Preference.OnPreferenceChangeListener {
    private static final String PREF_LOCATION_CHANGE = "location_change";

    private PrefServiceBridge mPrefServiceBridge;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPrefServiceBridge = PrefServiceBridge.getInstance();

        getActivity().setTitle(R.string.menu_downloads);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.download_preferences);

        updateSummaries();
        getLocationOptions();
    }

    public void updateSummaries() {
        PrefServiceBridge prefServiceBridge = PrefServiceBridge.getInstance();

        Preference locationChangePref = (Preference) findPreference(PREF_LOCATION_CHANGE);
        if (locationChangePref != null) {
            locationChangePref.setSummary(prefServiceBridge.getDownloadDefaultDirectory());
        }
    }

    private void getLocationOptions() {
        // Currently assuming that there are only two possible options: internal and external.
        ListPreference locationsPreference = (ListPreference) findPreference(PREF_LOCATION_CHANGE);

        int numLocations = hasExternalStorage() ? 2 : 1;
        CharSequence[] keys = new String[numLocations];
        CharSequence[] descriptions = new String[numLocations];

        keys[0] = "internal";
        descriptions[0] = "Internal";
        if (hasExternalStorage()) {
            keys[1] = "external";
            descriptions[1] = "External";
        }

        locationsPreference.setEntryValues(keys);
        locationsPreference.setEntries(descriptions);
        int index = isExternalStorageDefault() ? 1 : 0;
        locationsPreference.setValueIndex(index);
        locationsPreference.setOnPreferenceChangeListener(this);
    }

    private boolean hasExternalStorage() {
        // TODO(jming): Confirm this works on the other end for phones with SD cards.
        return Environment.isExternalStorageRemovable() && !Environment.isExternalStorageEmulated();
    }

    private boolean isExternalStorageDefault() {
        return Environment.getExternalStorageDirectory().equals(
                new File(mPrefServiceBridge.getDownloadDefaultDirectory()));
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_LOCATION_CHANGE.equals(preference.getKey())) {
            updateStorageLocation((String) newValue);
        }
        return true;
    }

    private void updateStorageLocation(String newValue) {
        File directory = newValue.equals("external") ? Environment.getExternalStorageDirectory()
                                                     : getDefaultAndroidDownloadDirectory();
        mPrefServiceBridge.setDownloadDefaultDirectory(directory.getAbsolutePath());

        updateSummaries();
    }

    private File getDefaultAndroidDownloadDirectory() {
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
    }
}
