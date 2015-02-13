// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Context;
import android.os.Build;

import org.chromium.base.CalledByNative;
import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.LibraryProcessType;;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.browser.preferences.LocationSettings;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.chrome.browser.preferences.ProtectedContentPreferences;
import org.chromium.chrome.browser.preferences.autofill.AutofillPreferences;
import org.chromium.chrome.browser.preferences.password.ManageSavedPasswordsPreferences;
import org.chromium.content.app.ContentApplication;
import org.chromium.content.browser.BrowserStartupController;

/**
 * Basic application functionality that should be shared among all browser applications that use
 * chrome layer.
 */
public abstract class ChromiumApplication extends ContentApplication {

    /**
     * Returns whether the Activity is being shown in multi-window mode.
     */
    public boolean isMultiWindow(Activity activity) {
        return false;
    }

    /**
     * Returns the class name of the Settings activity.
     */
    public abstract String getSettingsActivityName();

    /**
     * Opens a protected content settings page, if available.
     */
    @CalledByNative
    protected void openProtectedContentSettings() {
        assert Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;
        PreferencesLauncher.launchSettingsPage(this,
                ProtectedContentPreferences.class.getName());
    }

    @CalledByNative
    protected void showAutofillSettings() {
        PreferencesLauncher.launchSettingsPage(this,
                AutofillPreferences.class.getName());
    }

    @CalledByNative
    protected void showPasswordSettings() {
        PreferencesLauncher.launchSettingsPage(this,
                ManageSavedPasswordsPreferences.class.getName());
    }

    /**
     * For extending classes to carry out tasks that initialize the browser process.
     * Should be called almost immediately after the native library has loaded to initialize things
     * that really, really have to be set up early.  Avoid putting any long tasks here.
     */
    public void initializeProcess() { }

    /**
     * Start the browser process asynchronously. This will set up a queue of UI
     * thread tasks to initialize the browser process.
     *
     * Note that this can only be called on the UI thread.
     *
     * @param callback the callback to be called when browser startup is complete.
     * @throws ProcessInitException
     */
    public void startChromeBrowserProcessesAsync(BrowserStartupController.StartupCallback callback)
            throws ProcessInitException {
        assert ThreadUtils.runningOnUiThread() : "Tried to start the browser on the wrong thread";
        Context applicationContext = getApplicationContext();
        BrowserStartupController.get(applicationContext, LibraryProcessType.PROCESS_BROWSER)
                .startBrowserProcessesAsync(callback);
    }

    /**
     * Returns an instance of LocationSettings to be installed as a singleton.
     */
    public LocationSettings createLocationSettings() {
        return new LocationSettings(this);
    }

    /**
     * Opens the UI to clear browsing data.
     * @param tab The tab that triggered the request.
     */
    @CalledByNative
    protected void openClearBrowsingData(Tab tab) {}

    /**
     * @return Whether parental controls are enabled.  Returning true will disable
     *         incognito mode.
     */
    @CalledByNative
    protected abstract boolean areParentalControlsEnabled();

    // TODO(yfriedman): This is too widely available. Plumb this through ChromeNetworkDelegate
    // instead.
    protected abstract PKCS11AuthenticationManager getPKCS11AuthenticationManager();

    /**
     * @return The user agent string of Chrome.
     */
    public static String getBrowserUserAgent() {
        return nativeGetBrowserUserAgent();
    }

    private static native String nativeGetBrowserUserAgent();
}
