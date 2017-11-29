// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;
import android.view.View;
import android.view.View.OnClickListener;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreference;
import org.chromium.chrome.browser.preferences.ManagedPreferenceDelegate;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;

/**
 * Settings fragment that displays information about Chrome languages, which allow users to
 * seamlessly find and manage their languages preferences across platforms.
 */
public class LanguagesPreferences extends PreferenceFragment implements OnClickListener {
    static final int REQUEST_CODE_ADD_LANGUAGES = 1;
    static final String INTENT_LANGAUGE_ADDED = "LanguagesPreferences.LangaugeAdded";

    // The keys for each prefernce shown on the langauges page.
    static final String PREFERRED_LANGUAGES_KEY = "preferred_languages";
    static final String TRANSLATE_SWITCH_KEY = "translate_switch";

    private LanguageListPreference mLanguageListPref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActivity().setTitle(R.string.prefs_languages);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.languages_preferences);

        mLanguageListPref = (LanguageListPreference) findPreference(PREFERRED_LANGUAGES_KEY);
        mLanguageListPref.setAddButtonClickListener(this);

        ChromeSwitchPreference translateSwitch =
                (ChromeSwitchPreference) findPreference(TRANSLATE_SWITCH_KEY);
        boolean isTranslateEnabled = PrefServiceBridge.getInstance().isTranslateEnabled();
        translateSwitch.setChecked(isTranslateEnabled);

        translateSwitch.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                PrefServiceBridge.getInstance().setTranslateEnabled((boolean) newValue);
                return true;
            }
        });
        translateSwitch.setManagedPreferenceDelegate(new ManagedPreferenceDelegate() {
            @Override
            public boolean isPreferenceControlledByPolicy(Preference preference) {
                return PrefServiceBridge.getInstance().isTranslateManaged();
            }
        });
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_ADD_LANGUAGES) {
            if (resultCode == getActivity().RESULT_OK) {
                String code = data.getStringExtra(INTENT_LANGAUGE_ADDED);
                LanguagesManager.getInstance().addToAcceptLanguages(code);
            }
        }
    }

    @Override
    public void onClick(View v) {
        // Launch preference activity with LanguageSelectionFragment.
        Intent intent = PreferencesLauncher.createIntentForSettingsPage(
                getActivity(), LanguageSelectionFragment.class.getName());
        startActivityForResult(intent, REQUEST_CODE_ADD_LANGUAGES);
    }
}
