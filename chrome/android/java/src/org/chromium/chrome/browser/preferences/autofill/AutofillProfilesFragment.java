// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

/**
 * Autofill profiles fragment, which allows the user to edit autofill profiles.
 */
public class AutofillProfilesFragment
        extends PreferenceFragment implements PersonalDataManager.PersonalDataManagerObserver {
    static final String PREF_NEW_PROFILE = "new_profile";
    static final String PREF_EDIT_PROFILE = "edit_profile";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PreferenceUtils.addPreferencesFromResource(
                this, R.xml.autofill_and_payments_preference_fragment_screen);
        getActivity().setTitle(R.string.autofill_profiles_title);
    }

    @Override
    public void onResume() {
        super.onResume();
        // Always rebuild our list of profiles.  Although we could detect if profiles are added or
        // deleted (GUID list changes), the profile summary (name+addr) might be different.  To be
        // safe, we update all.
        rebuildProfileList();
    }

    private void rebuildProfileList() {
        getPreferenceScreen().removeAll();
        getPreferenceScreen().setOrderingAsAdded(true);

        for (AutofillProfile profile : PersonalDataManager.getInstance().getProfilesForSettings()) {
            // Add a preference for the profile.
            Preference pref;
            if (profile.getIsLocal()) {
                AutofillProfileEditorPreference localPref =
                        new AutofillProfileEditorPreference(getActivity());
                localPref.setTitle(profile.getFullName());
                localPref.setSummary(profile.getLabel());
                pref = localPref;
            } else {
                pref = new Preference(getActivity());
                pref.setWidgetLayoutResource(R.layout.autofill_server_data_label);
                pref.setFragment(AutofillServerProfilePreferences.class.getName());
            }
            pref.setKey(PREF_EDIT_PROFILE);
            Bundle args = pref.getExtras();
            args.putString(AutofillAndPaymentsPreferences.AUTOFILL_GUID, profile.getGUID());
            getPreferenceScreen().addPreference(pref);
        }

        // Add 'Add address' button. Tap of it brings up address editor which allows users type in
        // new addresses.
        AutofillProfileEditorPreference pref = new AutofillProfileEditorPreference(getActivity());
        Drawable plusIcon = ApiCompatibilityUtils.getDrawable(getResources(), R.drawable.plus);
        plusIcon.mutate();
        plusIcon.setColorFilter(
                ApiCompatibilityUtils.getColor(getResources(), R.color.pref_accent_color),
                PorterDuff.Mode.SRC_IN);
        pref.setIcon(plusIcon);
        pref.setTitle(R.string.autofill_create_profile);
        pref.setKey(PREF_NEW_PROFILE);
        getPreferenceScreen().addPreference(pref);
    }

    @Override
    public void onPersonalDataChanged() {
        rebuildProfileList();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        PersonalDataManager.getInstance().registerDataObserver(this);
    }

    @Override
    public void onDestroyView() {
        PersonalDataManager.getInstance().unregisterDataObserver(this);
        super.onDestroyView();
    }
}
