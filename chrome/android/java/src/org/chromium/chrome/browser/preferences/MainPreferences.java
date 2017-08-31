// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.preference.Preference;
import android.preference.PreferenceFragment;

import org.chromium.base.BuildInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.PasswordUIView;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.partnercustomizations.HomepageManager;
import org.chromium.chrome.browser.preferences.datareduction.DataReductionPreferences;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.search_engines.TemplateUrlService.TemplateUrl;
import org.chromium.chrome.browser.signin.SigninManager;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

/**
 * The main settings screen, shown when the user first opens Settings.
 */
public class MainPreferences extends PreferenceFragment
        implements SigninManager.SignInStateObserver, Preference.OnPreferenceClickListener,
                   TemplateUrlService.LoadListener {
    public static final String PREF_SIGN_IN = "sign_in";
    public static final String PREF_AUTOFILL_SETTINGS = "autofill_settings";
    public static final String PREF_SEARCH_ENGINE = "search_engine";
    public static final String PREF_SAVED_PASSWORDS = "saved_passwords";
    public static final String PREF_HOMEPAGE = "homepage";
    public static final String PREF_DATA_REDUCTION = "data_reduction";
    public static final String PREF_NOTIFICATIONS = "notifications";

    private final ManagedPreferenceDelegate mManagedPreferenceDelegate;
    private final SigninManager mSigninManager;

    private @Nullable SignInPreference mSignInPreference;
    private final List<Preference> mAllPreferences = new ArrayList<>();

    public MainPreferences() {
        setHasOptionsMenu(true);
        mManagedPreferenceDelegate = createManagedPreferenceDelegate();
        mSigninManager = SigninManager.get(getActivity());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        loadPreferences();
    }

    @Override
    public void onResume() {
        super.onResume();
        updatePreferences();

        if (mSigninManager.isSigninSupported()) {
            mSigninManager.addSignInStateObserver(this);
            mSignInPreference.registerForUpdates();
        }
    }

    @Override
    public void onPause() {
        super.onPause();

        if (mSigninManager.isSigninSupported()) {
            mSigninManager.removeSignInStateObserver(this);
            mSignInPreference.unregisterForUpdates();
        }
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        Intent intent = new Intent(
                  Intent.ACTION_VIEW,
                  Uri.parse(PasswordUIView.getAccountDashboardURL()));
        intent.setPackage(getActivity().getPackageName());
        getActivity().startActivity(intent);
        return true;
    }

    private void loadPreferences() {
        PreferenceUtils.addPreferencesFromResource(this, R.xml.main_preferences);

        savePreferences();

        mSignInPreference = (SignInPreference) findPreference(PREF_SIGN_IN);

        ChromeBasePreference autofillPref =
                (ChromeBasePreference) findPreference(PREF_AUTOFILL_SETTINGS);
        autofillPref.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        ChromeBasePreference searchEnginePref =
                (ChromeBasePreference) findPreference(PREF_SEARCH_ENGINE);
        searchEnginePref.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        ChromeBasePreference passwordsPref =
                (ChromeBasePreference) findPreference(PREF_SAVED_PASSWORDS);
        passwordsPref.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        ChromeBasePreference dataReduction =
                (ChromeBasePreference) findPreference(PREF_DATA_REDUCTION);
        dataReduction.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        if (BuildInfo.isAtLeastO()) {
            // If we are on Android O+ the Notifications preference should lead to the Android
            // Settings notifications page, not to Chrome's notifications settings page.
            Preference notifications = findPreference(PREF_NOTIFICATIONS);
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
            getPreferenceScreen().removePreference(findPreference(PREF_NOTIFICATIONS));
        }
    }

    /**
     * Stores all preferences in memory so that, if they needed to be added/removed from the
     * Preference Screen, there would be no need to reload them from 'main_preferences.xml'.
     */
    private void savePreferences() {
        int prefCount = getPreferenceScreen().getPreferenceCount();
        for (int index = 0; index < prefCount; index++) {
            mAllPreferences.add(getPreferenceScreen().getPreference(index));
        }
    }

    private void updatePreferences() {
        if (TemplateUrlService.getInstance().isLoaded()) {
            updateSummary();
        } else {
            TemplateUrlService.getInstance().registerLoadListener(this);
            TemplateUrlService.getInstance().load();
            ChromeBasePreference searchEnginePref =
                    (ChromeBasePreference) findPreference(PREF_SEARCH_ENGINE);
            searchEnginePref.setEnabled(false);
        }

        ChromeBasePreference passwordsPref =
                (ChromeBasePreference) findPreference(PREF_SAVED_PASSWORDS);
        setOnOffSummary(
                passwordsPref, PrefServiceBridge.getInstance().isRememberPasswordsEnabled());

        if (HomepageManager.shouldShowHomepageSetting()) {
            addPreferenceIfAbsent(PREF_HOMEPAGE);
            Preference homepagePref = findPreference(PREF_HOMEPAGE);
            setOnOffSummary(homepagePref,
                    HomepageManager.getInstance(getActivity()).getPrefHomepageEnabled());
        } else {
            removePreferenceIfPresent(PREF_HOMEPAGE);
        }

        ChromeBasePreference dataReduction =
                (ChromeBasePreference) findPreference(PREF_DATA_REDUCTION);
        dataReduction.setSummary(DataReductionPreferences.generateSummary(getResources()));

        if (mSigninManager.isSigninSupported()) {
            addPreferenceIfAbsent(PREF_SIGN_IN);
        } else {
            removePreferenceIfPresent(PREF_SIGN_IN);
        }
    }

    private void addPreferenceIfAbsent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference == null) {
            getPreferenceScreen().addPreference(getLoadedPreference(key));
        }
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
    }

    private Preference getLoadedPreference(String key) {
        for (Preference preference : mAllPreferences) {
            if (preference.getKey().equals(key)) {
                return preference;
            }
        }

        assert false : "Unexpected preference key: " + key;
        return null;
    }

    private void setOnOffSummary(Preference pref, boolean isOn) {
        pref.setSummary(getResources().getString(isOn ? R.string.text_on : R.string.text_off));
    }

    private void updateSummary() {
        ChromeBasePreference searchEnginePref =
                (ChromeBasePreference) findPreference(PREF_SEARCH_ENGINE);
        searchEnginePref.setEnabled(true);

        String defaultSearchEngineName = null;
        TemplateUrl dseTemplateUrl =
                TemplateUrlService.getInstance().getDefaultSearchEngineTemplateUrl();
        if (dseTemplateUrl != null) defaultSearchEngineName = dseTemplateUrl.getShortName();
        searchEnginePref.setSummary(defaultSearchEngineName);
    }

    // SigninManager.SignInStateObserver implementation.

    @Override
    public void onSignedIn() {
        // After signing in or out of a managed account, preferences may change or become enabled
        // or disabled.
        new Handler().post(this ::updatePreferences);
    }

    @Override
    public void onSignedOut() {
        updatePreferences();
    }

    // TemplateUrlService.LoadListener implementation.

    @Override
    public void onTemplateUrlServiceLoaded() {
        TemplateUrlService.getInstance().unregisterLoadListener(this);
        updateSummary();
    }

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
