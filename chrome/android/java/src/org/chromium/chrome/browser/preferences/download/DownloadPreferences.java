// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

/**
 * Fragment to keep track of all downloads related preferences.
 */
public class DownloadPreferences extends PreferenceFragment {
    private static final String PREF_LOCATION_CHANGE = "location_change";

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActivity().setTitle(R.string.menu_downloads);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.download_preferences);

        updateSummaries();
    }

    public void updateSummaries() {
        PrefServiceBridge prefServiceBridge = PrefServiceBridge.getInstance();

        Preference locationChangePref = (Preference) findPreference(PREF_LOCATION_CHANGE);
        if (locationChangePref != null) {
            locationChangePref.setSummary(prefServiceBridge.getDownloadDefaultDirectory());
        }
    }
}
