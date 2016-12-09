// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.content.ContentResolver;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.provider.Settings;
import android.text.TextUtils;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.UrlUtilities;
import org.chromium.webapk.lib.client.WebApkVersion;

import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * WebApkUpdateManager manages when to check for updates to the WebAPK's Web Manifest, and sends
 * an update request to the WebAPK Server when an update is needed.
 */
public class WebApkUpdateManager implements WebApkUpdateDataFetcher.Observer {
    private static final String TAG = "WebApkUpdateManager";

    /** Number of milliseconds between checks for whether the WebAPK's Web Manifest has changed. */
    public static final long FULL_CHECK_UPDATE_INTERVAL = TimeUnit.DAYS.toMillis(3L);

    /**
     * Number of milliseconds to wait before re-requesting an updated WebAPK from the WebAPK
     * server if the previous update attempt failed.
     */
    public static final long RETRY_UPDATE_DURATION = TimeUnit.HOURS.toMillis(12L);

    /** Data extracted from the WebAPK's launch intent and from the WebAPK's Android Manifest. */
    private WebApkInfo mInfo;

    /**
     * Whether the previous WebAPK update succeeded. True if there has not been any update attempts.
     */
    private boolean mPreviousUpdateSucceeded;

    private WebApkUpdateDataFetcher mFetcher;

    /**
     * Checks whether the WebAPK's Web Manifest has changed. Requests an updated WebAPK if the
     * Web Manifest has changed. Skips the check if the check was done recently.
     * @param tab  The tab of the WebAPK.
     * @param info The WebApkInfo of the WebAPK.
     */
    public void updateIfNeeded(Tab tab, WebApkInfo info) {
        mInfo = info;

        WebappDataStorage storage = WebappRegistry.getInstance().getWebappDataStorage(mInfo.id());
        mPreviousUpdateSucceeded = didPreviousUpdateSucceed(storage);

        if (!shouldCheckIfWebManifestUpdated(storage, mInfo, mPreviousUpdateSucceeded)) return;

        mFetcher = buildFetcher();
        mFetcher.start(tab, mInfo, this);
    }

    public void destroy() {
        destroyFetcher();
    }

    @Override
    public void onFinishedFetchingWebManifestForInitialUrl(
            WebApkInfo fetchedInfo, String bestIconUrl) {
        onGotManifestData(fetchedInfo, bestIconUrl);
    }

    @Override
    public void onGotManifestData(WebApkInfo fetchedInfo, String bestIconUrl) {
        WebappDataStorage storage = WebappRegistry.getInstance().getWebappDataStorage(mInfo.id());
        storage.updateTimeOfLastCheckForUpdatedWebManifest();

        boolean gotManifest = (fetchedInfo != null);
        boolean needsUpgrade = isShellApkVersionOutOfDate(mInfo)
                || (gotManifest && needsUpdate(mInfo, fetchedInfo, bestIconUrl));
        Log.v(TAG, "Got Manifest: " + gotManifest);
        Log.v(TAG, "WebAPK upgrade needed: " + needsUpgrade);

        // If the Web Manifest was not found and an upgrade is requested, stop fetching Web
        // Manifests as the user navigates to avoid sending multiple WebAPK update requests. In
        // particular:
        // - A WebAPK update request on the initial load because the Shell APK version is out of
        //   date.
        // - A second WebAPK update request once the user navigates to a page which points to the
        //   correct Web Manifest URL because the Web Manifest has been updated by the Web
        //   developer.
        //
        // If the Web Manifest was not found and an upgrade is not requested, keep on fetching
        // Web Manifests as the user navigates. For instance, the WebAPK's start_url might not
        // point to a Web Manifest because start_url redirects to the WebAPK's main page.
        if (gotManifest || needsUpgrade) {
            destroyFetcher();
        }

        if (!needsUpgrade) {
            if (!mPreviousUpdateSucceeded) {
                recordUpdate(storage, true);
            }
            return;
        }

        // Set WebAPK update as having failed in case that Chrome is killed prior to
        // {@link onBuiltWebApk} being called.
        recordUpdate(storage, false);

        if (fetchedInfo != null) {
            updateAsync(fetchedInfo, bestIconUrl);
            return;
        }

        // Since we could not fetch the Web Manifest, we do not know what the best icon URL is. Pass
        // an empty "best icon URL" to tell the server that there is no best icon URL.
        updateAsync(mInfo, "");
    }

    /**
     * Builds {@link WebApkUpdateDataFetcher}. In a separate function for the sake of tests.
     */
    protected WebApkUpdateDataFetcher buildFetcher() {
        return new WebApkUpdateDataFetcher();
    }

    /**
     * Sends request to WebAPK Server to update WebAPK.
     */
    protected void updateAsync(WebApkInfo info, String bestIconUrl) {
        int versionCode = readVersionCodeFromAndroidManifest(info.webApkPackageName());
        String bestIconMurmur2Hash = info.iconUrlToMurmur2HashMap().get(bestIconUrl);
        nativeUpdateAsync(info.id(), info.manifestStartUrl(), info.scopeUri().toString(),
                info.name(), info.shortName(), bestIconUrl, bestIconMurmur2Hash, info.icon(),
                info.iconUrlToMurmur2HashMap().keySet().toArray(new String[0]), info.displayMode(),
                info.orientation(), info.themeColor(), info.backgroundColor(), info.manifestUrl(),
                info.webApkPackageName(), versionCode);
    }

    /**
     * Destroys {@link mFetcher}. In a separate function for the sake of tests.
     */
    protected void destroyFetcher() {
        if (mFetcher == null) return;

        mFetcher.destroy();
        mFetcher = null;
    }

    /** Returns the current time. In a separate function for the sake of testing. */
    protected long currentTimeMillis() {
        return System.currentTimeMillis();
    }

    /**
     * Reads the WebAPK's version code. Returns 0 on failure.
     */
    private int readVersionCodeFromAndroidManifest(String webApkPackage) {
        try {
            PackageManager packageManager =
                    ContextUtils.getApplicationContext().getPackageManager();
            PackageInfo packageInfo = packageManager.getPackageInfo(webApkPackage, 0);
            return packageInfo.versionCode;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return 0;
    }

    /**
     * Returns whether the previous WebAPK update attempt succeeded. Returns true if there has not
     * been any update attempts.
     */
    private static boolean didPreviousUpdateSucceed(WebappDataStorage storage) {
        long lastUpdateCompletionTime = storage.getLastWebApkUpdateRequestCompletionTime();
        if (lastUpdateCompletionTime == WebappDataStorage.LAST_USED_INVALID
                || lastUpdateCompletionTime == WebappDataStorage.LAST_USED_UNSET) {
            return true;
        }
        return storage.getDidLastWebApkUpdateRequestSucceed();
    }

    /**
     * Whether there is a new version of the //chrome/android/webapk/shell_apk code.
     */
    private static boolean isShellApkVersionOutOfDate(WebApkInfo info) {
        return info.shellApkVersion() < WebApkVersion.CURRENT_SHELL_APK_VERSION;
    }

    /**
     * Returns whether the Web Manifest should be refetched to check whether it has been updated.
     * TODO: Make this method static once there is a static global clock class.
     * @param storage WebappDataStorage with the WebAPK's cached data.
     * @param info Meta data from WebAPK's Android Manifest.
     * @param previousUpdateSucceeded Whether the previous update attempt succeeded.
     * True if there has not been any update attempts.
     */
    private boolean shouldCheckIfWebManifestUpdated(
            WebappDataStorage storage, WebApkInfo info, boolean previousUpdateSucceeded) {
        if (CommandLine.getInstance().hasSwitch(
                    ChromeSwitches.CHECK_FOR_WEB_MANIFEST_UPDATE_ON_STARTUP)) {
            return true;
        }

        // Updating WebAPKs requires "installation from unknown sources" to be enabled. It is
        // confusing for a user to see a dialog asking them to enable "installation from unknown
        // sources" when they are in the middle of using the WebAPK (as opposed to after requesting
        // to add a WebAPK to the homescreen).
        if (!installingFromUnknownSourcesAllowed()) {
            return false;
        }

        if (isShellApkVersionOutOfDate(info)) return true;

        long now = currentTimeMillis();
        long sinceLastCheckDurationMs = now - storage.getLastCheckForWebManifestUpdateTime();
        if (sinceLastCheckDurationMs >= FULL_CHECK_UPDATE_INTERVAL) return true;

        long sinceLastUpdateRequestDurationMs =
                now - storage.getLastWebApkUpdateRequestCompletionTime();
        return sinceLastUpdateRequestDurationMs >= RETRY_UPDATE_DURATION
                && !previousUpdateSucceeded;
    }

    /**
     * Updates {@link WebappDataStorage} with the time of the latest WebAPK update and whether the
     * WebAPK update succeeded.
     */
    private static void recordUpdate(WebappDataStorage storage, boolean success) {
        // Update the request time and result together. It prevents getting a correct request time
        // but a result from the previous request.
        storage.updateTimeOfLastWebApkUpdateRequestCompletion();
        storage.updateDidLastWebApkUpdateRequestSucceed(success);
    }

    /**
     * Returns whether the user has enabled installing apps from sources other than the Google
     * Play Store.
     */
    private static boolean installingFromUnknownSourcesAllowed() {
        ContentResolver contentResolver = ContextUtils.getApplicationContext().getContentResolver();
        try {
            int setting = Settings.Secure.getInt(
                    contentResolver, Settings.Secure.INSTALL_NON_MARKET_APPS);
            return setting == 1;
        } catch (Settings.SettingNotFoundException e) {
            return false;
        }
    }

    /**
     * Checks whether the WebAPK needs to be updated.
     * @param info               Meta data from WebAPK's Android Manifest.
     * @param fetchedInfo        Fetched data for Web Manifest.
     * @param bestFetchedIconUrl The icon URL in {@link fetchedInfo#iconUrlToMurmur2HashMap()} best
     *                           suited for use as the launcher icon on this device.
     */
    private boolean needsUpdate(WebApkInfo info, WebApkInfo fetchedInfo, String bestIconUrl) {
        // We should have computed the Murmur2 hash for the bitmap at the best icon URL for
        // {@link fetchedInfo} (but not the other icon URLs.)
        String fetchedBestIconMurmur2Hash = fetchedInfo.iconUrlToMurmur2HashMap().get(bestIconUrl);
        String bestIconMurmur2Hash =
                findMurmur2HashForUrlIgnoringFragment(mInfo.iconUrlToMurmur2HashMap(), bestIconUrl);

        return !TextUtils.equals(bestIconMurmur2Hash, fetchedBestIconMurmur2Hash)
                || !urlsMatchIgnoringFragments(
                           mInfo.scopeUri().toString(), fetchedInfo.scopeUri().toString())
                || !urlsMatchIgnoringFragments(
                           mInfo.manifestStartUrl(), fetchedInfo.manifestStartUrl())
                || !TextUtils.equals(mInfo.shortName(), fetchedInfo.shortName())
                || !TextUtils.equals(mInfo.name(), fetchedInfo.name())
                || mInfo.backgroundColor() != fetchedInfo.backgroundColor()
                || mInfo.themeColor() != fetchedInfo.themeColor()
                || mInfo.orientation() != fetchedInfo.orientation()
                || mInfo.displayMode() != fetchedInfo.displayMode();
    }

    /**
     * Returns the Murmur2 hash for entry in {@link iconUrlToMurmur2HashMap} whose canonical
     * representation, ignoring fragments, matches {@link iconUrlToMatch}.
     */
    private String findMurmur2HashForUrlIgnoringFragment(
            Map<String, String> iconUrlToMurmur2HashMap, String iconUrlToMatch) {
        for (Map.Entry<String, String> entry : iconUrlToMurmur2HashMap.entrySet()) {
            if (urlsMatchIgnoringFragments(entry.getKey(), iconUrlToMatch)) {
                return entry.getValue();
            }
        }
        return null;
    }

    /**
     * Returns whether the urls match ignoring fragments. Canonicalizes the URLs prior to doing the
     * comparison.
     */
    protected boolean urlsMatchIgnoringFragments(String url1, String url2) {
        return UrlUtilities.urlsMatchIgnoringFragments(url1, url2);
    }

    /**
     * Called after either a request to update the WebAPK has been sent or the update process
     * fails.
     */
    @CalledByNative
    private static void onBuiltWebApk(String id, boolean success) {
        WebappDataStorage storage = WebappRegistry.getInstance().getWebappDataStorage(id);
        recordUpdate(storage, success);
    }

    private static native void nativeUpdateAsync(String id, String startUrl, String scope,
            String name, String shortName, String bestIconUrl, String bestIconMurmur2Hash,
            Bitmap bestIcon, String[] iconUrls, int displayMode, int orientation, long themeColor,
            long backgroundColor, String manifestUrl, String webApkPackage, int webApkVersion);
}
