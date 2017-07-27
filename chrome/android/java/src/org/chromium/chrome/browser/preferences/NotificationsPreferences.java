// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.os.Build;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.preferences.website.ContentSettingsResources;

/**
 * Settings fragment that allows the user to configure notifications. It contains general
 * notification channels at the top level and links to website specific notifications. This is only
 * used on pre-O devices, devices on Android O+ will link to the Android notification settings.
 */
public class NotificationsPreferences extends PreferenceFragment {
    private static final String PREF_FROM_WEBSITES = "from_websites";
    private static final String PREF_SUGGESTIONS = "content_suggestions";

    private ChromeSwitchPreference mSuggestionsPref;
    private Preference mFromWebsitesPref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // TODO(peconn): Link to here from settings.
        assert Build.VERSION.SDK_INT < 26 : "NotificationsPreferences should only be used pre-O.";

        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.notifications_preferences);
        getActivity().setTitle(R.string.prefs_notifications);

        mSuggestionsPref = (ChromeSwitchPreference) findPreference(PREF_SUGGESTIONS);
        mSuggestionsPref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                SnippetsBridge.setContentSuggestionsNotificationsEnabled((boolean) newValue);
                return true;
            }
        });

        mFromWebsitesPref = findPreference(PREF_FROM_WEBSITES);

        update();
    }

    @Override
    public void onResume() {
        super.onResume();
        update();
    }

    /**
     * Updates the state of displayed preferences.
     */
    private void update() {
        mFromWebsitesPref.setSummary(ContentSettingsResources.getCategorySummary(
                ContentSettingsType.CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
                PrefServiceBridge.getInstance().isNotificationsEnabled()));

        mSuggestionsPref.setChecked(SnippetsBridge.areContentSuggestionsNotificationsEnabled());
    }
}
