// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;
import org.chromium.chrome.browser.widget.TintedDrawable;

/**
 * Settings fragment that displays information about Chrome languages, which allow users to
 * seamlessly find and manage their languages preferences across platforms.
 */
public class LanguagesPreferences extends PreferenceFragment {
    // The keys for each prefernce shown on the langauges page.
    static final String ACCEPT_LANGUAGES_KEY = "accept_languages";
    static final String ADD_LANGUAGE_KEY = "add_language";
    static final String TRANSLATE_KEY = "translate";

    private LanguageListPreference mLanguageListPref;
    private Preference mAddLanguagePref;
    private Preference mTranslatePref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActivity().setTitle(R.string.prefs_languages);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.languages_preferences);

        mLanguageListPref = (LanguageListPreference) findPreference(ACCEPT_LANGUAGES_KEY);
        mAddLanguagePref = findPreference(ADD_LANGUAGE_KEY);
        mTranslatePref = findPreference(TRANSLATE_KEY);

        updatePreferenceStates();
    }

    @Override
    public void onResume() {
        super.onResume();
        updatePreferenceStates();
    }

    private void updatePreferenceStates() {
        // "Translate" preference.
        if (mTranslatePref != null) {
            setTranslateStateSummary();
        }

        // "Add language" preference.
        if (mAddLanguagePref != null && mAddLanguagePref.getIcon() == null) {
            mAddLanguagePref.setIcon(TintedDrawable.constructTintedDrawable(
                    getResources(), R.drawable.plus, R.color.pref_accent_color));
        }

        // TODO(crbug/783049): Preferred languages list preference.
    }

    private void setTranslateStateSummary() {
        if (mTranslatePref != null) {
            boolean translateEnabled = PrefServiceBridge.getInstance().isTranslateEnabled();
            mTranslatePref.setSummary(translateEnabled ? R.string.languages_offer_translate_on
                                                       : R.string.languages_offer_translate_off);
        }
    }
}
