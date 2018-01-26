// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.v7.app.AppCompatActivity;

import org.chromium.base.Log;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.browser.bookmarks.BookmarkActivity;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.ui.display.DisplayAndroid;

/**
 * Ensures that the native library is loaded by synchronously initializing it on creation.
 *
 * This is needed for Activities that can be started without going through the regular asynchronous
 * browser startup pathway, which could happen if the user restarted Chrome after it died in the
 * background with the Activity visible.  One example is {@link BookmarkActivity} and its kin.
 */
public abstract class SynchronousInitializationActivity extends AppCompatActivity {
    private static final String TAG = "SyncInitActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Ensure that native library is loaded.
        try {
            ChromeBrowserInitializer.getInstance(this).handleSynchronousStartup();
        } catch (ProcessInitException e) {
            Log.e(TAG, "Failed to start browser process.", e);
            ChromeApplication.reportStartupErrorAndExit(e);
        }
    }

    @CallSuper
    @Override
    @TargetApi(Build.VERSION_CODES.N)
    protected void attachBaseContext(Context context) {
        super.attachBaseContext(context);

        // Work-around smallestScreenWidthDp sometimes being set wrong, causing loaded resources
        // to not match up with isTablet().
        // See crbug.com/588838, crbug.com/662338, crbug.com/780593.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            DisplayAndroid display = DisplayAndroid.getNonMultiDisplay(context);
            Configuration overrideConfiguration = new Configuration();
            // Force the activity's ResourceManager's concept of isTablet() to match DisplayAndroid.
            overrideConfiguration.smallestScreenWidthDp =
                    (int) (display.getSmallestWidth() / display.getDipScale() + 0.5f);
            applyOverrideConfiguration(overrideConfiguration);
        }
    }
}
