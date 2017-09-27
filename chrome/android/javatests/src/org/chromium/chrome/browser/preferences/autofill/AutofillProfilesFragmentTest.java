// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.preferences.Preferences;
import org.chromium.chrome.browser.preferences.PreferencesTest;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;

/**
 * Unit test suite for AutofillProfilesFragment.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class AutofillProfilesFragmentTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    // testNoAdded + Add.
    // thenTestAlready + edit + delete
    // simply cancel
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testNoAddedProfileOnList() throws Exception {
        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());

        AutofillTestHelper helper = new AutofillTestHelper();
        String profile1 = helper.setProfile(new AutofillProfile("", "https://example.com", true,
                "Jon Doe", "Google", "340 Main St", "CA", "Los Angeles", "", "90291", "", "US",
                "650-253-0000", "jon.doe@gmail.com", "en-US"));
        String profile2 = helper.setProfile(new AutofillProfile("", "https://example.com", true,
                "Rob Doe", "Google", "340 Main St", "CA", "Los Angeles", "", "90291", "", "US",
                "650-253-0000", "jon.doe@gmail.com", "en-US"));
        String profile3 = helper.setProfile(new AutofillProfile("", "https://example.com", true,
                "Tom Doe", "Google", "340 Main St", "CA", "Los Angeles", "", "90291", "", "US",
                "650-253-0000", "jon.doe@gmail.com", "en-US"));

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                final PreferenceFragment fragment =
                        (PreferenceFragment) preferences.getFragmentForTest();
                AutofillProfilesFragment autofillProfileFragment =
                        (AutofillProfilesFragment) preferences.getFragmentForTest();
                Preference addProfile =
                        fragment.findPreference(AutofillProfilesFragment.PREF_NEW_PROFILE);

                PreferenceScreen screen = autofillProfileFragment.getPreferenceScreen();
                int preferenceCount = screen.getPreferenceCount();
                System.out.println("Parastoo: preferenceCount: " + preferenceCount);
                // for (int i = 0; i < preferenceCount; i++) {
                //     System.out.println("Parastoo: curKey: " + screen.getPreference(i).getKey());
                // }

                // 3 for 3 profiles and 1 for the add button.
                Assert.assertTrue(preferenceCount, 4);
                Assert.assertTrue(addProfile != null);
                PreferencesTest.clickPreference(autofillProfileFragment, addProfile);

                preferences.finish();
            }
        });
    }
}
