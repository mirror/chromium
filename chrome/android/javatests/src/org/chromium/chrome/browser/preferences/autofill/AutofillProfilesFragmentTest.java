// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.preference.PreferenceFragment;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.preferences.Preferences;
import org.chromium.chrome.browser.preferences.PreferencesTest;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * Unit test suite for AutofillProfilesFragment.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class AutofillProfilesFragmentTest {
    @Rule
    public final AutofillTestRule rule = new AutofillTestRule();
    private int mPreferenceCount = 1; // For Add Profile Button

    @Before
    public void setUp() throws InterruptedException, ExecutionException, TimeoutException {
        AutofillTestHelper helper = new AutofillTestHelper();
        helper.setProfile(new AutofillProfile("", "https://example.com", true, "First Profile",
                "Google", "111 First St", "CA", "Los Angeles", "", "90291", "", "US",
                "650-253-0000", "first@gmail.com", "en-US"));
        helper.setProfile(new AutofillProfile("", "https://example.com", true, "Second Profile",
                "Google", "111 Second St", "CA", "Los Angeles", "", "90291", "", "US",
                "650-253-0000", "second@gmail.com", "en-US"));
        mPreferenceCount += 2; // Each preference represents a profile.
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testAddProfile() throws Exception {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());
        AutofillProfilesFragment autofillProfileFragment =
                (AutofillProfilesFragment) activity.getFragmentForTest();

        // Check the preferences on the initial screen.
        Assert.assertEquals(mPreferenceCount,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference addProfile =
                (AutofillProfileEditorPreference) fragment.findPreference(
                        AutofillProfilesFragment.PREF_NEW_PROFILE);
        Assert.assertTrue(addProfile != null);

        // Add a profile.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferencesTest.clickPreference(autofillProfileFragment, addProfile);
                rule.setEditorDialog(
                        ((AutofillProfileEditorPreference) addProfile).getEditorDialog());
                try {
                    rule.setTextInEditorAndWait(
                            new String[] {"Added Profile", "Google", "111 Added St", "Los Angeles",
                                    "CA", "90291", "650-253-0000", "add@profile.com"});
                    rule.clickInEditorAndWait(R.id.payments_edit_done_button);
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
        // Check if the preferences are updated correctly.
        rule.waitForThePreferenceUpdate();
        mPreferenceCount++;
        Assert.assertEquals(mPreferenceCount,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        AutofillProfileEditorPreference addedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Added Profile");
        Assert.assertTrue(addedProfile != null);
        System.out.println("Parastoo: added profile summary: " + addedProfile.getSummary());
        Assert.assertEquals("111 Added St, 90291", addedProfile.getSummary());
        activity.finish();
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testDeleteProfile() throws Exception {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());
        AutofillProfilesFragment autofillProfileFragment =
                (AutofillProfilesFragment) activity.getFragmentForTest();

        // Check the preferences on the initial screen.
        Assert.assertEquals(mPreferenceCount,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference firstProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("First Profile");
        Assert.assertTrue(firstProfile != null);
        Assert.assertEquals("First Profile", firstProfile.getTitle());

        // Delete a profile.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferencesTest.clickPreference(autofillProfileFragment, firstProfile);
                rule.setEditorDialog(
                        ((AutofillProfileEditorPreference) firstProfile).getEditorDialog());
                try {
                    rule.clickInEditorAndWait(R.id.delete_menu_id);
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
        // Check if the preferences are updated correctly.
        rule.waitForThePreferenceUpdate();
        mPreferenceCount--;
        Assert.assertEquals(mPreferenceCount,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        AutofillProfileEditorPreference remainedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Second Profile");
        Assert.assertTrue(remainedProfile != null);
        AutofillProfileEditorPreference deletedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("First Profile");
        Assert.assertTrue(deletedProfile == null);
        activity.finish();
    }

    @Test
    @MediumTest
    @Feature({"Preferences"})
    public void testEditProfile() throws Exception {
        Preferences activity =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        AutofillProfilesFragment.class.getName());
        AutofillProfilesFragment autofillProfileFragment =
                (AutofillProfilesFragment) activity.getFragmentForTest();

        // Check the preferences on the initial screen.
        Assert.assertEquals(mPreferenceCount,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        PreferenceFragment fragment = (PreferenceFragment) activity.getFragmentForTest();
        AutofillProfileEditorPreference secondProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Second Profile");
        Assert.assertTrue(secondProfile != null);
        Assert.assertEquals("Second Profile", secondProfile.getTitle());

        // Edit a profile.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferencesTest.clickPreference(autofillProfileFragment, secondProfile);
                rule.setEditorDialog(
                        ((AutofillProfileEditorPreference) secondProfile).getEditorDialog());
                try {
                    rule.setTextInEditorAndWait(new String[] {"Edited Profile", "Google",
                            "111 Edited St", "Los Angeles", "CA", "90291", "650-253-0000",
                            "edit@profile.com"});
                    rule.clickInEditorAndWait(R.id.payments_edit_done_button);
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        });
        // Check if the preferences are updated correctly.
        rule.waitForThePreferenceUpdate();
        Assert.assertEquals(mPreferenceCount,
                autofillProfileFragment.getPreferenceScreen().getPreferenceCount());
        AutofillProfileEditorPreference editedProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Edited Profile");
        Assert.assertTrue(editedProfile != null);
        Assert.assertEquals("111 Edited St, 90291", editedProfile.getSummary());
        AutofillProfileEditorPreference oldProfile =
                (AutofillProfileEditorPreference) fragment.findPreference("Second Profile");
        Assert.assertTrue(oldProfile == null);
        activity.finish();
    }
}
