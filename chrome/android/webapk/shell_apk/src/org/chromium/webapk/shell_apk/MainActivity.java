// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;

/**
 * WebAPK's main Activity.
 */
public class MainActivity extends HostBrowserLaunchingActivity {
    private static final String TAG = "cr_MainActivity";

    /**
     * The URL to launch the WebAPK. Used in the case of deep-links (intents from other apps) that
     * fall into the WebAPK scope.
     */
    private String mOverrideUrl;

    /**
     * The "start_url" baked in the AndroidManifest.xml.
     */
    private String mStartUrl;

    @Override
    protected boolean canHandleIntent(Bundle savedInstanceState) {
        Bundle metadata = WebApkUtils.readMetaData(this);

        mOverrideUrl = getOverrideUrl();
        mStartUrl = (mOverrideUrl != null) ? mOverrideUrl
                                           : metadata.getString(WebApkMetaDataKeys.START_URL);
        mStartUrl = WebApkUtils.rewriteIntentUrlIfNecessary(mStartUrl, metadata);
        Log.v(TAG, "Url of the WebAPK: " + mStartUrl);

        if (mStartUrl == null) {
            return false;
        }
        return true;
    }

    @Override
    protected Intent getLaunchIntent() {
        boolean forceNavigation = false;
        int source = getIntent().getIntExtra(WebApkConstants.EXTRA_SOURCE, 0);
        if (mOverrideUrl != null) {
            if (source == WebApkConstants.SHORTCUT_SOURCE_UNKNOWN) {
                source = WebApkConstants.SHORTCUT_SOURCE_EXTERNAL_INTENT;
            }
            forceNavigation =
                    getIntent().getBooleanExtra(WebApkConstants.EXTRA_FORCE_NAVIGATION, true);
        }

        // The override URL is non null when the WebAPK is launched from a deep link. The WebAPK
        // should navigate to the URL in the deep link even if the WebAPK is already open.
        Intent intent = new Intent();
        intent.setAction(ACTION_START_WEBAPK);
        intent.putExtra(WebApkConstants.EXTRA_URL, mStartUrl)
                .putExtra(WebApkConstants.EXTRA_SOURCE, source)
                .putExtra(WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME, getPackageName())
                .putExtra(WebApkConstants.EXTRA_FORCE_NAVIGATION, forceNavigation);
        return intent;
    }

    /**
     * Retrieves URL from the intent's data. Returns null if a URL could not be retrieved.
     */
    private String getOverrideUrl() {
        String overrideUrl = getIntent().getDataString();
        if (overrideUrl != null
                && (overrideUrl.startsWith("https:") || overrideUrl.startsWith("http:"))) {
            return overrideUrl;
        }
        return null;
    }
}