// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import static android.support.test.espresso.action.ViewActions.click;
import static android.support.test.espresso.action.ViewActions.closeSoftKeyboard;
import static android.support.test.espresso.action.ViewActions.scrollTo;
import static android.support.test.espresso.action.ViewActions.typeText;
import static android.support.test.espresso.assertion.ViewAssertions.doesNotExist;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.RootMatchers.withDecorView;
import static android.support.test.espresso.matcher.ViewMatchers.isDisplayed;
import static android.support.test.espresso.matcher.ViewMatchers.isEnabled;
import static android.support.test.espresso.matcher.ViewMatchers.withContentDescription;
import static android.support.test.espresso.matcher.ViewMatchers.withId;
import static android.support.test.espresso.matcher.ViewMatchers.withParent;
import static android.support.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.Matchers.allOf;
import static org.hamcrest.Matchers.containsString;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.not;
import static org.hamcrest.Matchers.startsWith;

import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.support.test.InstrumentationRegistry;
import android.support.test.espresso.Espresso;
import android.support.test.filters.SmallTest;
import android.support.test.rule.ActivityTestRule;
import android.view.View;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.PasswordManagerHandler;
import org.chromium.chrome.browser.SavedPasswordEntry;
import org.chromium.chrome.browser.preferences.ChromeBaseCheckBoxPreference;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreference;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.Preferences;
import org.chromium.chrome.browser.preferences.PreferencesTest;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * Tests for the "Save Passwords" settings screen.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class SavePasswordsPreferencesTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    @Rule
    public TestRule mProcessor = new Features.InstrumentationProcessor();

    @Rule
    final public ActivityTestRule<Preferences> mActivityTestRule =
            new ActivityTestRule<>(Preferences.class);

    private static final class FakePasswordManagerHandler implements PasswordManagerHandler {
        // This class has exactly one observer, set on construction and expected to last at least as
        // long as this object (a good candidate is the owner of this object).
        private final PasswordListObserver mObserver;

        // The faked contents of the password store to be displayed.
        private ArrayList<SavedPasswordEntry> mSavedPasswords = new ArrayList<SavedPasswordEntry>();

        // This is set to true when serializePasswords is called.
        private boolean mSerializePasswordsCalled;

        public void setSavedPasswords(ArrayList<SavedPasswordEntry> savedPasswords) {
            mSavedPasswords = savedPasswords;
        }

        public boolean getSerializePasswordsCalled() {
            return mSerializePasswordsCalled;
        }

        /**
         * Constructor.
         * @param PasswordListObserver The only observer.
         */
        public FakePasswordManagerHandler(PasswordListObserver observer) {
            mObserver = observer;
        }

        // Pretends that the updated lists are |mSavedPasswords| for the saved passwords and an
        // empty list for exceptions and immediately calls the observer.
        @Override
        public void updatePasswordLists() {
            mObserver.passwordListAvailable(mSavedPasswords.size());
        }

        @Override
        public SavedPasswordEntry getSavedPasswordEntry(int index) {
            return mSavedPasswords.get(index);
        }

        @Override
        public String getSavedPasswordException(int index) {
            // Define this method before starting to use it in tests.
            assert false;
            return null;
        }

        @Override
        public void removeSavedPasswordEntry(int index) {
            // Define this method before starting to use it in tests.
            assert false;
            return;
        }

        @Override
        public void removeSavedPasswordException(int index) {
            // Define this method before starting to use it in tests.
            assert false;
            return;
        }

        @Override
        public void serializePasswords(Callback<String> callback) {
            mSerializePasswordsCalled = true;
        }
    }

    private final static SavedPasswordEntry ZEUS_ON_EARTH =
            new SavedPasswordEntry("http://www.phoenicia.gr", "Zeus", "Europa");
    private final static SavedPasswordEntry ARES_AT_OLYMP =
            new SavedPasswordEntry("https://1-of-12.olymp.gr", "Ares", "God-o'w@r");
    private final static SavedPasswordEntry PHOBOS_AT_OLYMP =
            new SavedPasswordEntry("https://visitor.olymp.gr", "Phobos-son-of-ares", "G0d0fF34r");
    private final static SavedPasswordEntry DEIMOS_AT_OLYMP =
            new SavedPasswordEntry("https://visitor.olymp.gr", "Deimops-Ares-son", "G0d0fT3rr0r");
    private final static SavedPasswordEntry HADES_AT_UNDERWORLD =
            new SavedPasswordEntry("https://underworld.gr", "", "C3rb3rus");
    private final static SavedPasswordEntry[] GREEK_GODS = {
            ZEUS_ON_EARTH, ARES_AT_OLYMP, PHOBOS_AT_OLYMP, DEIMOS_AT_OLYMP, HADES_AT_UNDERWORLD,
    };

    // Used to provide fake lists of stored passwords. Tests which need it can use setPasswordSource
    // to instantiate it.
    FakePasswordManagerHandler mHandler;

    /**
     * Helper to set up a fake source of displayed passwords.
     * @param entry An entry to be added to saved passwords. Can be null.
     */
    private void setPasswordSource(SavedPasswordEntry entry) throws Exception {
        SavedPasswordEntry[] entries = {};
        if (entry != null) {
            entries = new SavedPasswordEntry[] {entry};
        }
        setPasswordSourceWithMultipleEntries(entries);
    }

    /**
     * Helper to set up a fake source of displayed passwords with multiple initial passwords.
     * @param initialEntries All entries to be added to saved passwords. Can not be null.
     */
    private void setPasswordSourceWithMultipleEntries(SavedPasswordEntry[] initialEntries)
            throws Exception {
        if (mHandler == null) {
            mHandler = new FakePasswordManagerHandler(PasswordManagerHandlerProvider.getInstance());
        }
        ArrayList<SavedPasswordEntry> entries =
                new ArrayList<SavedPasswordEntry>(Arrays.asList(initialEntries));
        mHandler.setSavedPasswords(entries);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PasswordManagerHandlerProvider.getInstance().setPasswordManagerHandlerForTest(
                        mHandler);
            }
        });
    }

    /**
     * Helper to rotate the screen. Landscape -> Portrait or Portrait -> Landscape - depends on
     * current orientation.
     */
    private void rotateScreen() {
        final int orientation = InstrumentationRegistry.getTargetContext()
                                        .getResources()
                                        .getConfiguration()
                                        .orientation;

        mActivityTestRule.getActivity().setRequestedOrientation(
                (orientation == Configuration.ORIENTATION_PORTRAIT)
                        ? ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
                        : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    /**
     * Ensure that resetting of empty passwords list works.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testResetListEmpty() throws Exception {
        // Load the preferences, they should show the empty list.
        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                SavePasswordsPreferences savePasswordPreferences =
                        (SavePasswordsPreferences) preferences.getFragmentForTest();
                // Emulate an update from PasswordStore. This should not crash.
                savePasswordPreferences.passwordListAvailable(0);
            }
        });
    }

    /**
     * Ensure that the on/off switch in "Save Passwords" settings actually enables and disables
     * password saving.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testSavePasswordsSwitch() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PrefServiceBridge.getInstance().setRememberPasswordsEnabled(true);
            }
        });

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                SavePasswordsPreferences savedPasswordPrefs =
                        (SavePasswordsPreferences) preferences.getFragmentForTest();
                ChromeSwitchPreference onOffSwitch = (ChromeSwitchPreference)
                        savedPasswordPrefs.findPreference(
                                SavePasswordsPreferences.PREF_SAVE_PASSWORDS_SWITCH);
                Assert.assertTrue(onOffSwitch.isChecked());

                PreferencesTest.clickPreference(savedPasswordPrefs, onOffSwitch);
                Assert.assertFalse(PrefServiceBridge.getInstance().isRememberPasswordsEnabled());
                PreferencesTest.clickPreference(savedPasswordPrefs, onOffSwitch);
                Assert.assertTrue(PrefServiceBridge.getInstance().isRememberPasswordsEnabled());

                preferences.finish();

                PrefServiceBridge.getInstance().setRememberPasswordsEnabled(false);
            }
        });

        final Preferences preferences2 =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                SavePasswordsPreferences savedPasswordPrefs =
                        (SavePasswordsPreferences) preferences2.getFragmentForTest();
                ChromeSwitchPreference onOffSwitch = (ChromeSwitchPreference)
                        savedPasswordPrefs.findPreference(
                                SavePasswordsPreferences.PREF_SAVE_PASSWORDS_SWITCH);
                Assert.assertFalse(onOffSwitch.isChecked());
            }
        });
    }

    /**
     * Ensure that the "Auto Sign-in" switch in "Save Passwords" settings actually enables and
     * disables auto sign-in.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testAutoSignInCheckbox() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PrefServiceBridge.getInstance().setPasswordManagerAutoSigninEnabled(true);
            }
        });

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                SavePasswordsPreferences passwordPrefs =
                        (SavePasswordsPreferences) preferences.getFragmentForTest();
                ChromeBaseCheckBoxPreference onOffSwitch =
                        (ChromeBaseCheckBoxPreference) passwordPrefs.findPreference(
                                SavePasswordsPreferences.PREF_AUTOSIGNIN_SWITCH);
                Assert.assertTrue(onOffSwitch.isChecked());

                PreferencesTest.clickPreference(passwordPrefs, onOffSwitch);
                Assert.assertFalse(
                        PrefServiceBridge.getInstance().isPasswordManagerAutoSigninEnabled());
                PreferencesTest.clickPreference(passwordPrefs, onOffSwitch);
                Assert.assertTrue(
                        PrefServiceBridge.getInstance().isPasswordManagerAutoSigninEnabled());

                preferences.finish();

                PrefServiceBridge.getInstance().setPasswordManagerAutoSigninEnabled(false);
            }
        });

        final Preferences preferences2 =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                SavePasswordsPreferences passwordPrefs =
                        (SavePasswordsPreferences) preferences2.getFragmentForTest();
                ChromeBaseCheckBoxPreference onOffSwitch =
                        (ChromeBaseCheckBoxPreference) passwordPrefs.findPreference(
                                SavePasswordsPreferences.PREF_AUTOSIGNIN_SWITCH);
                Assert.assertFalse(onOffSwitch.isChecked());
            }
        });
    }

    /**
     * Check that if there are no saved passwords, the export menu item is disabled.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures("PasswordExport")
    public void testExportMenuDisabled() throws Exception {
        // Ensure there are no saved passwords reported to settings.
        setPasswordSource(null);

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        // The text matches a text view, but the disabled entity is some wrapper two levels up in
        // the view hierarchy, hence the two withParent matchers.
        Espresso.onView(allOf(withText(R.string.save_password_preferences_export_action_title),
                                withParent(withParent(not(isEnabled())))))
                .check(matches(isDisplayed()));
    }

    /**
     * Check that if there are saved passwords, the export menu item is enabled.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures("PasswordExport")
    public void testExportMenuEnabled() throws Exception {
        setPasswordSource(new SavedPasswordEntry("https://example.com", "test user", "password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        // The text matches a text view, but the potentially disabled entity is some wrapper two
        // levels up in the view hierarchy, hence the two withParent matchers.
        Espresso.onView(allOf(withText(R.string.save_password_preferences_export_action_title),
                                withParent(withParent(isEnabled()))))
                .check(matches(isDisplayed()));
    }

    /**
     * Check that if "PasswordExport" feature is not explicitly enabled, there is no menu item to
     * export passwords.
     * TODO(crbug.com/788701): Add the @DisableFeatures annotation once exporting gets enabled by
     * default, and remove completely once the feature is gone.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testExportMenuMissing() throws Exception {
        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        // Ideally this would need the same matcher (Espresso.OVERFLOW_BUTTON_MATCHER) as used
        // inside Espresso.openActionBarOverflowOrOptionsMenu(), but that is private to Espresso.
        // Matching the overflow menu with the class name "OverflowMenuButton" won't work on
        // obfuscated release builds, so matching the description remains. The
        // OVERFLOW_BUTTON_MATCHER specifies the string directly, not via string resource, so this
        // is also done below.
        Espresso.onView(withContentDescription("More options")).check(doesNotExist());
    }

    /**
     * Check that tapping the export menu requests the passwords to be serialised in the background.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures("PasswordExport")
    public void testExportTriggersSerialization() throws Exception {
        setPasswordSource(new SavedPasswordEntry("https://example.com", "test user", "password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setScreenLockSetUpOverride(
                ReauthenticationManager.OverrideState.AVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        // Before tapping the menu item for export, pretend that the last successful
        // reauthentication just happened. This will allow the export flow to continue.
        ReauthenticationManager.setLastReauthTimeMillis(System.currentTimeMillis());
        Espresso.onView(withText(R.string.save_password_preferences_export_action_title))
                .perform(click());

        Assert.assertTrue(mHandler.getSerializePasswordsCalled());
    }

    /**
     * Check that the export menu item is included and hidden behind the overflow menu. Check that
     * the menu displays the warning before letting the user export passwords.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures("PasswordExport")
    public void testExportMenuItem() throws Exception {
        setPasswordSource(new SavedPasswordEntry("https://example.com", "test user", "password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setScreenLockSetUpOverride(
                ReauthenticationManager.OverrideState.AVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        // Before tapping the menu item for export, pretend that the last successful
        // reauthentication just happened. This will allow the export flow to continue.
        ReauthenticationManager.setLastReauthTimeMillis(System.currentTimeMillis());
        Espresso.onView(withText(R.string.save_password_preferences_export_action_title))
                .perform(click());

        Espresso.onView(withText(R.string.settings_passwords_export_description))
                .check(matches(isDisplayed()));
    }

    /**
     * Check whether the user is asked to set up a screen lock if attempting to export passwords.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures("PasswordExport")
    public void testExportMenuItemNoLock() throws Exception {
        setPasswordSource(new SavedPasswordEntry("https://example.com", "test user", "password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setScreenLockSetUpOverride(
                ReauthenticationManager.OverrideState.UNAVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        View mainDecorView = preferences.getWindow().getDecorView();
        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        Espresso.onView(withText(R.string.save_password_preferences_export_action_title))
                .perform(click());
        Espresso.onView(withText(R.string.password_export_set_lock_screen))
                .inRoot(withDecorView(not(is(mainDecorView))))
                .check(matches(isDisplayed()));
    }

    /**
     * Check that if exporting is cancelled for the absence of the screen lock, the menu item is
     * enabled for a retry.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures("PasswordExport")
    public void testExportMenuItemReenabledNoLock() throws Exception {
        setPasswordSource(new SavedPasswordEntry("https://example.com", "test user", "password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setScreenLockSetUpOverride(
                ReauthenticationManager.OverrideState.UNAVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        Espresso.onView(withText(R.string.save_password_preferences_export_action_title))
                .perform(click());
        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        // The text matches a text view, but the potentially disabled entity is some wrapper two
        // levels up in the view hierarchy, hence the two withParent matchers.
        Espresso.onView(allOf(withText(R.string.save_password_preferences_export_action_title),
                                withParent(withParent(isEnabled()))))
                .check(matches(isDisplayed()));
    }

    /**
     * Check that if exporting is cancelled for the user's failure to reauthenticate, the menu item
     * is enabled for a retry.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures("PasswordExport")
    public void testExportMenuItemReenabledReauthFailure() throws Exception {
        setPasswordSource(new SavedPasswordEntry("https://example.com", "test user", "password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setSkipSystemReauth(true);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        Espresso.onView(withText(R.string.save_password_preferences_export_action_title))
                .perform(click());
        // The reauthentication dialog is skipped and the last reauthentication timestamp is not
        // reset. This looks like a failed reauthentication to SavePasswordsPreferences' onResume.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                preferences.getFragmentForTest().onResume();
            }
        });
        Espresso.openActionBarOverflowOrOptionsMenu(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        // The text matches a text view, but the potentially disabled entity is some wrapper two
        // levels up in the view hierarchy, hence the two withParent matchers.
        Espresso.onView(allOf(withText(R.string.save_password_preferences_export_action_title),
                                withParent(withParent(isEnabled()))))
                .check(matches(isDisplayed()));
    }

    /**
     * Check whether the user is asked to set up a screen lock if attempting to view passwords.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testViewPasswordNoLock() throws Exception {
        setPasswordSource(new SavedPasswordEntry("https://example.com", "test user", "password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setScreenLockSetUpOverride(
                ReauthenticationManager.OverrideState.UNAVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        View mainDecorView = preferences.getWindow().getDecorView();
        Espresso.onView(withText(containsString("test user"))).perform(click());
        Espresso.onView(withContentDescription(R.string.password_entry_editor_copy_stored_password))
                .perform(click());
        Espresso.onView(withText(R.string.password_entry_editor_set_lock_screen))
                .inRoot(withDecorView(not(is(mainDecorView))))
                .check(matches(isDisplayed()));
    }

    /**
     * Check whether the user can view a saved password.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testViewPassword() throws Exception {
        setPasswordSource(
                new SavedPasswordEntry("https://example.com", "test user", "test password"));

        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setScreenLockSetUpOverride(
                ReauthenticationManager.OverrideState.AVAILABLE);

        final Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        SavePasswordsPreferences.class.getName());

        Espresso.onView(withText(containsString("test user"))).perform(click());

        // Before tapping the view button, pretend that the last successful reauthentication just
        // happened. This will allow the export flow to continue.
        ReauthenticationManager.setLastReauthTimeMillis(System.currentTimeMillis());
        Espresso.onView(withContentDescription(R.string.password_entry_editor_view_stored_password))
                .perform(click());
        Espresso.onView(withText("test password")).check(matches(isDisplayed()));
    }

    /**
     * Check that the search item is visible if the Feature is enabled.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchIconVisibleWithFeature() throws Exception {
        setPasswordSource(null); // Initialize empty preferences.
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        Espresso.onView(withId(R.id.menu_id_search)).check(matches(isDisplayed()));
    }

    /**
     * Check that the search item is not visible if the Feature is disabled.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchIconGoneWithoutFeature() throws Exception {
        setPasswordSource(null); // Initialize empty preferences.
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        Espresso.onView(withId(R.id.menu_id_search)).check(doesNotExist());
    }

    /**
     * Check that the search filters the list by name.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchFiltersByUserName() throws Exception {
        setPasswordSourceWithMultipleEntries(GREEK_GODS);
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        // Search for a string matching multiple user names. Case doesn't need to match.
        Espresso.onView(withId(R.id.menu_id_search)).perform(click());
        Espresso.onView(withId(R.id.search_src_text))
                .perform(click(), typeText("aREs"), closeSoftKeyboard());

        Espresso.onView(withText(ARES_AT_OLYMP.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(PHOBOS_AT_OLYMP.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(DEIMOS_AT_OLYMP.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(ZEUS_ON_EARTH.getUserName())).check(doesNotExist());
        Espresso.onView(withText(HADES_AT_UNDERWORLD.getUrl())).check(doesNotExist());
    }

    /**
     * Check that the search filters the list by URL.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchFiltersByUrl() throws Exception {
        setPasswordSourceWithMultipleEntries(GREEK_GODS);
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        // Search for a string that matches multiple URLs. Case doesn't need to match.
        Espresso.onView(withId(R.id.menu_id_search)).perform(click());
        Espresso.onView(withId(R.id.search_src_text))
                .perform(click(), typeText("Olymp"), closeSoftKeyboard());

        Espresso.onView(withText(ARES_AT_OLYMP.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(PHOBOS_AT_OLYMP.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(DEIMOS_AT_OLYMP.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(ZEUS_ON_EARTH.getUserName())).check(doesNotExist());
        Espresso.onView(withText(HADES_AT_UNDERWORLD.getUrl())).check(doesNotExist());
    }

    /**
     * Check that triggering the search hides all non-password prefs.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchIconClickedHidesGeneralPrefs() throws Exception {
        setPasswordSourceWithMultipleEntries(GREEK_GODS);
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        Espresso.onView(withText(R.string.passwords_auto_signin_title))
                .check(matches(isDisplayed()));
        Espresso.onView(withText(startsWith("View and manage"))).check(matches(isDisplayed()));

        Espresso.onView(withId(R.id.menu_id_search)).perform(click());

        Espresso.onView(withText(R.string.passwords_auto_signin_title)).check(doesNotExist());
        Espresso.onView(withText(startsWith("View and manage"))).check(doesNotExist());
    }

    /**
     * Check that closing the search via back button brings back all non-password prefs.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchBarBackButtonBringsBackGeneralPrefs() throws Exception {
        setPasswordSourceWithMultipleEntries(GREEK_GODS);
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        Espresso.onView(withId(R.id.menu_id_search)).perform(click());

        Espresso.onView(withText(R.string.passwords_auto_signin_title)).check(doesNotExist());
        Espresso.onView(withText(startsWith("View and manage"))).check(doesNotExist());

        Espresso.pressBack(); // Close keyboard.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        Espresso.onView(withContentDescription("Collapse")).perform(click());

        Espresso.onView(withText(R.string.passwords_auto_signin_title))
                .check(matches(isDisplayed()));
        Espresso.onView(withText(startsWith("View and manage"))).check(matches(isDisplayed()));
    }

    /**
     * Check that closing the search via back button brings back all non-password prefs.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchBarBackKeyBringsBackGeneralPrefs() throws Exception {
        setPasswordSourceWithMultipleEntries(GREEK_GODS);
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        Espresso.onView(withId(R.id.menu_id_search)).perform(click());

        Espresso.onView(withText(R.string.passwords_auto_signin_title)).check(doesNotExist());
        Espresso.onView(withText(startsWith("View and manage"))).check(doesNotExist());

        Espresso.pressBack(); // Close keyboard.
        Espresso.pressBack(); // Close search view.

        Espresso.onView(withText(R.string.passwords_auto_signin_title))
                .check(matches(isDisplayed()));
        Espresso.onView(withText(startsWith("View and manage"))).check(matches(isDisplayed()));
    }

    /**
     * Check that the filtered password list persists after the user had inspected a single result.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.PASSWORD_SEARCH)
    public void testSearchResultsPersistAfterEntryInspection() throws Exception {
        setPasswordSourceWithMultipleEntries(GREEK_GODS);
        PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                SavePasswordsPreferences.class.getName());

        // Open the search and filter all but "Zeus".
        Espresso.onView(withId(R.id.menu_id_search)).perform(click());
        Espresso.onView(withId(R.id.search_src_text))
                .perform(click(), typeText("Zeu"), closeSoftKeyboard());

        Espresso.onView(withText(R.string.passwords_auto_signin_title)).check(doesNotExist());
        Espresso.onView(withText(ZEUS_ON_EARTH.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(PHOBOS_AT_OLYMP.getUserName())).check(doesNotExist());
        Espresso.onView(withText(HADES_AT_UNDERWORLD.getUrl())).check(doesNotExist());

        // Click "Zeus" to open edit field and verify the password.
        Espresso.onView(withText(ZEUS_ON_EARTH.getUserName())).perform(click());
        // Before tapping the view button, pretend that the last successful reauthentication just
        // happened.
        ReauthenticationManager.setApiOverride(ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setScreenLockSetUpOverride(
                ReauthenticationManager.OverrideState.AVAILABLE);
        ReauthenticationManager.setLastReauthTimeMillis(System.currentTimeMillis());
        Espresso.onView(withId(R.id.password_entry_editor_view_password))
                .perform(scrollTo(), click());
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        ;
        Espresso.onView(withText(ZEUS_ON_EARTH.getPassword())).check(matches(isDisplayed()));
        Espresso.pressBack(); // Go back to the search list.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        ;

        Espresso.onView(withText(R.string.passwords_auto_signin_title)).check(doesNotExist());
        Espresso.onView(withText(ZEUS_ON_EARTH.getUserName())).check(matches(isDisplayed()));
        Espresso.onView(withText(PHOBOS_AT_OLYMP.getUserName())).check(doesNotExist());
        Espresso.onView(withText(HADES_AT_UNDERWORLD.getUrl())).check(doesNotExist());
    }
}
