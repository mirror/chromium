// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;

import org.chromium.base.BuildInfo;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.PasswordUIView;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.partnercustomizations.HomepageManager;
import org.chromium.chrome.browser.preferences.autofill.AutofillAndPaymentsPreferences;
import org.chromium.chrome.browser.preferences.datareduction.DataReductionPreferences;
import org.chromium.chrome.browser.preferences.password.SavePasswordsPreferences;
import org.chromium.chrome.browser.preferences.privacy.PrivacyPreferences;
import org.chromium.chrome.browser.preferences.website.SiteSettingsPreferences;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.search_engines.TemplateUrlService.TemplateUrl;
import org.chromium.chrome.browser.signin.SigninManager;

import java.util.HashMap;
import java.util.Map;

/**
 * The main settings screen, shown when the user first opens Settings.
 */
public class MainPreferences extends PreferenceFragment
        implements SigninManager.SignInStateObserver, Preference.OnPreferenceClickListener,
                   TemplateUrlService.LoadListener {
    public static final String PREF_SIGN_IN = "sign_in";
    public static final String PREF_BASICS_SECTION = "basics_section";
    public static final String PREF_SEARCH_ENGINE = "search_engine";
    public static final String PREF_AUTOFILL_SETTINGS = "autofill_settings";
    public static final String PREF_SAVED_PASSWORDS = "saved_passwords";
    public static final String PREF_NOTIFICATIONS = "notifications";
    public static final String PREF_HOMEPAGE = "homepage";
    public static final String PREF_ADVANCED_SECTION = "advanced_section";
    public static final String PREF_PRIVACY = "privacy";
    public static final String PREF_ACCESSIBILITY = "accessibility";
    public static final String PREF_SITE_SETTINGS = "site_settings";
    public static final String PREF_DATA_REDUCTION = "data_reduction";
    public static final String PREF_ABOUT_CHROME = "about_chrome";

    private final ManagedPreferenceDelegate mManagedPreferenceDelegate;

    private Map<String, Integer> mPreferenceOrder = new HashMap<>();
    private @Nullable SignInPreference mSignInPreference;

    public MainPreferences() {
        setHasOptionsMenu(true);
        mManagedPreferenceDelegate = createManagedPreferenceDelegate();
        setupPreferenceOrder();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(getActivity()));
    }

    @Override
    public void onResume() {
        super.onResume();
        updatePreferences();

        if (SigninManager.get(getActivity()).isSigninSupported()) {
            SigninManager.get(getActivity()).addSignInStateObserver(this);
            mSignInPreference.registerForUpdates();
        }
    }

    @Override
    public void onPause() {
        super.onPause();

        if (SigninManager.get(getActivity()).isSigninSupported()) {
            SigninManager.get(getActivity()).removeSignInStateObserver(this);
            mSignInPreference.unregisterForUpdates();
        }
    }

    private void setupPreferenceOrder() {
        int order = 0;
        mPreferenceOrder.put(PREF_SIGN_IN, order++);
        mPreferenceOrder.put(PREF_BASICS_SECTION, order++);
        mPreferenceOrder.put(PREF_SEARCH_ENGINE, order++);
        mPreferenceOrder.put(PREF_AUTOFILL_SETTINGS, order++);
        mPreferenceOrder.put(PREF_SAVED_PASSWORDS, order++);
        mPreferenceOrder.put(PREF_NOTIFICATIONS, order++);
        mPreferenceOrder.put(PREF_HOMEPAGE, order++);
        mPreferenceOrder.put(PREF_ADVANCED_SECTION, order++);
        mPreferenceOrder.put(PREF_PRIVACY, order++);
        mPreferenceOrder.put(PREF_ACCESSIBILITY, order++);
        mPreferenceOrder.put(PREF_SITE_SETTINGS, order++);
        mPreferenceOrder.put(PREF_DATA_REDUCTION, order++);
        mPreferenceOrder.put(PREF_ABOUT_CHROME, order);
    }

    private void updatePreferences() {
        setupSigninPreference();
        setupPreferenceCategory(PREF_BASICS_SECTION, R.string.prefs_section_basics);
        setupSearchEnginePreference();
        setupChromeBasePreference(PREF_AUTOFILL_SETTINGS, R.string.prefs_autofill_and_payments,
                AutofillAndPaymentsPreferences.class);
        setupPasswordPreference();
        setupNotificationPreference();
        setupHomepagePreference();
        setupPreferenceCategory(PREF_ADVANCED_SECTION, R.string.prefs_section_advanced);
        setupPreference(PREF_PRIVACY, R.string.prefs_privacy, PrivacyPreferences.class);
        setupPreference(
                PREF_ACCESSIBILITY, R.string.prefs_accessibility, AccessibilityPreferences.class);
        setupPreference(
                PREF_SITE_SETTINGS, R.string.prefs_site_settings, SiteSettingsPreferences.class);
        setupDataReductionPreference();
        setupPreference(
                PREF_ABOUT_CHROME, R.string.prefs_about_chrome, AboutChromePreferences.class);
    }

    private void setupSigninPreference() {
        if (SigninManager.get(getActivity()).isSigninSupported()) {
            if (mSignInPreference == null) {
                mSignInPreference = new SignInPreference(getActivity());
                setupPreferenceKeyOrderTitle(
                        mSignInPreference, PREF_SIGN_IN, R.string.sign_in_to_chrome);
                getPreferenceScreen().addPreference(mSignInPreference);
            }
        } else {
            getPreferenceScreen().removePreference(mSignInPreference);
            mSignInPreference = null;
        }
    }

    private void setupPreferenceCategory(String key, @StringRes int title) {
        PreferenceCategory preferenceCategory = (PreferenceCategory) findPreference(key);
        if (preferenceCategory != null) {
            return;
        }
        preferenceCategory = new PreferenceCategory(getActivity());
        setupPreferenceKeyOrderTitle(preferenceCategory, key, title);
        getPreferenceScreen().addPreference(preferenceCategory);
    }

    private Preference setupPreference(String key, @StringRes int title,
            Class<? extends PreferenceFragment> preferenceFragmentClass) {
        Preference preference = findPreference(key);

        if (preference == null) {
            preference = new Preference(getActivity());
            setupPreferenceKeyOrderTitle(preference, key, title);
            preference.setFragment(preferenceFragmentClass.getCanonicalName());
            getPreferenceScreen().addPreference(preference);
        }

        return preference;
    }

    private ChromeBasePreference setupChromeBasePreference(String key, @StringRes int title,
            Class<? extends PreferenceFragment> preferenceFragmentClass) {
        ChromeBasePreference chromeBasePreference = (ChromeBasePreference) findPreference(key);

        if (chromeBasePreference == null) {
            chromeBasePreference = new ChromeBasePreference(getActivity());
            setupPreferenceKeyOrderTitle(chromeBasePreference, key, title);
            chromeBasePreference.setFragment(preferenceFragmentClass.getCanonicalName());
            chromeBasePreference.setTitle(title);
            getPreferenceScreen().addPreference(chromeBasePreference);
        }

        return chromeBasePreference;
    }

    private void setupPreferenceKeyOrderTitle(
            Preference preference, String key, @StringRes int titleRes) {
        preference.setKey(key);
        preference.setOrder(mPreferenceOrder.get(key));
        preference.setTitle(titleRes);
    }

    private void setupSearchEnginePreference() {
        ChromeBasePreference searchEnginePref =
                (ChromeBasePreference) findPreference(PREF_SEARCH_ENGINE);
        if (searchEnginePref == null) {
            searchEnginePref = setupChromeBasePreference(
                    PREF_SEARCH_ENGINE, R.string.prefs_search_engine, SearchEnginePreference.class);
        }

        if (TemplateUrlService.getInstance().isLoaded()) {
            updateSearchEnginePreferenceSummary();
        } else {
            TemplateUrlService.getInstance().registerLoadListener(this);
            TemplateUrlService.getInstance().load();
            searchEnginePref.setEnabled(false);
        }
    }

    private void setupPasswordPreference() {
        ChromeBasePreference preference =
                (ChromeBasePreference) findPreference(PREF_SAVED_PASSWORDS);

        if (preference == null) {
            preference = setupChromeBasePreference(PREF_SAVED_PASSWORDS,
                    R.string.prefs_saved_passwords, SavePasswordsPreferences.class);
        }

        setOnOffSummary(preference, PrefServiceBridge.getInstance().isRememberPasswordsEnabled());
    }

    private void setupNotificationPreference() {
        if (BuildInfo.isAtLeastO()) {
            // If we are on Android O+ the Notifications preference should lead to the Android
            // Settings notifications page, not to Chrome's notifications settings page.
            Preference notifications = findPreference(PREF_NOTIFICATIONS);
            if (notifications == null) {
                notifications = setupPreference(PREF_NOTIFICATIONS, R.string.prefs_notifications,
                        NotificationsPreferences.class);
            }
            notifications.setOnPreferenceClickListener(preference -> {
                // TODO(crbug.com/707804): Use Android O constants.
                Intent intent = new Intent();
                intent.setAction("android.settings.APP_NOTIFICATION_SETTINGS");
                intent.putExtra("android.provider.extra.APP_PACKAGE", BuildInfo.getPackageName());
                startActivity(intent);
                // We handle the click so the default action (opening NotificationsPreference)
                // isn't triggered.
                return true;
            });
        } else if (!ChromeFeatureList.isEnabled(
                           ChromeFeatureList.CONTENT_SUGGESTIONS_NOTIFICATIONS)) {
            // The Notifications Preferences page currently only contains the Content Suggestions
            // Notifications setting and a link to per-website notification settings. The latter can
            // be access through Site Settings, so if the Content Suggestions Notifications feature
            // isn't enabled we don't show the Notifications Preferences page.

            // This checks whether the Content Suggestions Notifications *feature* is enabled on the
            // user's device, not whether the user has Content Suggestions Notifications themselves
            // enabled (which is what the user can toggle on the Notifications Preferences page).
            removePreferenceIfPresent(PREF_NOTIFICATIONS);
        }
    }

    private void setupHomepagePreference() {
        if (HomepageManager.shouldShowHomepageSetting()) {
            Preference preference = findPreference(PREF_HOMEPAGE);
            if (preference == null) {
                preference = setupPreference(
                        PREF_HOMEPAGE, R.string.options_homepage_title, HomepagePreferences.class);
            }
            setOnOffSummary(preference,
                    HomepageManager.getInstance(getActivity()).getPrefHomepageEnabled());
        } else {
            removePreferenceIfPresent(PREF_HOMEPAGE);
        }
    }

    private void setupDataReductionPreference() {
        Preference dataReductionPreference = findPreference(PREF_DATA_REDUCTION);

        if (dataReductionPreference == null) {
            dataReductionPreference = setupChromeBasePreference(PREF_DATA_REDUCTION,
                    R.string.data_reduction_title, DataReductionPreferences.class);
        }

        dataReductionPreference.setSummary(
                DataReductionPreferences.generateSummary(getResources()));
    }

    private void setOnOffSummary(Preference pref, boolean isOn) {
        pref.setSummary(getResources().getString(isOn ? R.string.text_on : R.string.text_off));
    }

    private void updateSearchEnginePreferenceSummary() {
        String defaultSearchEngineName = null;
        TemplateUrl dseTemplateUrl =
                TemplateUrlService.getInstance().getDefaultSearchEngineTemplateUrl();
        if (dseTemplateUrl != null) defaultSearchEngineName = dseTemplateUrl.getShortName();

        ChromeBasePreference searchEnginePref =
                (ChromeBasePreference) findPreference(PREF_SEARCH_ENGINE);
        searchEnginePref.setEnabled(true);
        searchEnginePref.setSummary(defaultSearchEngineName);
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
    }

    // SigninManager.SignInStateObserver implementation.
    @Override
    public void onSignedIn() {
        // After signing in or out of a managed account, preferences may change or become enabled
        // or disabled.
        new Handler().post(() -> updatePreferences());
    }

    @Override
    public void onSignedOut() {
        updatePreferences();
    }

    // TemplateUrlService.LoadListener implementation.
    @Override
    public void onTemplateUrlServiceLoaded() {
        TemplateUrlService.getInstance().unregisterLoadListener(this);
        updateSearchEnginePreferenceSummary();
    }

    // Preference.OnPreferenceClickListener implementation.
    @Override
    public boolean onPreferenceClick(Preference preference) {
        Intent intent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(PasswordUIView.getAccountDashboardURL()));
        intent.setPackage(getActivity().getPackageName());
        getActivity().startActivity(intent);
        return true;
    }

    @VisibleForTesting
    ManagedPreferenceDelegate getManagedPreferenceDelegateForTest() {
        return mManagedPreferenceDelegate;
    }

    private ManagedPreferenceDelegate createManagedPreferenceDelegate() {
        return new ManagedPreferenceDelegate() {
            @Override
            public boolean isPreferenceControlledByPolicy(Preference preference) {
                if (PREF_AUTOFILL_SETTINGS.equals(preference.getKey())) {
                    return PersonalDataManager.isAutofillManaged();
                }
                if (PREF_SAVED_PASSWORDS.equals(preference.getKey())) {
                    return PrefServiceBridge.getInstance().isRememberPasswordsManaged();
                }
                if (PREF_DATA_REDUCTION.equals(preference.getKey())) {
                    return DataReductionProxySettings.getInstance().isDataReductionProxyManaged();
                }
                if (PREF_SEARCH_ENGINE.equals(preference.getKey())) {
                    return TemplateUrlService.getInstance().isDefaultSearchManaged();
                }
                return false;
            }

            @Override
            public boolean isPreferenceClickDisabledByPolicy(Preference preference) {
                if (PREF_AUTOFILL_SETTINGS.equals(preference.getKey())) {
                    return PersonalDataManager.isAutofillManaged()
                            && !PersonalDataManager.isAutofillEnabled();
                }
                if (PREF_SAVED_PASSWORDS.equals(preference.getKey())) {
                    PrefServiceBridge prefs = PrefServiceBridge.getInstance();
                    return prefs.isRememberPasswordsManaged()
                            && !prefs.isRememberPasswordsEnabled();
                }
                if (PREF_DATA_REDUCTION.equals(preference.getKey())) {
                    DataReductionProxySettings settings = DataReductionProxySettings.getInstance();
                    return settings.isDataReductionProxyManaged()
                            && !settings.isDataReductionProxyEnabled();
                }
                if (PREF_SEARCH_ENGINE.equals(preference.getKey())) {
                    return TemplateUrlService.getInstance().isDefaultSearchManaged();
                }
                return super.isPreferenceClickDisabledByPolicy(preference);
            }
        };
    }
}