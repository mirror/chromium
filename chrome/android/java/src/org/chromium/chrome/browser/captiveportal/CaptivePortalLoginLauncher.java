// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.captiveportal;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.os.Build;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/** Launches the system captive portal sign-in app on Android. */
@JNINamespace("chrome::android")
public class CaptivePortalLoginLauncher {
    private static final String TAG = "PortalLoginLauncher";

    private static void launchSignInApp(Context context, Network network) {
        Intent intent = new Intent(android.net.ConnectivityManager.ACTION_CAPTIVE_PORTAL_SIGN_IN);
        intent.putExtra(ConnectivityManager.EXTRA_NETWORK, network);
        // NOTE: This is Missing EXTRA_CAPTIVE_PORTAL. It will launch the sign-in app, but the app
        // will crash if the user uses one of the menu options which references to this missing
        // CaptivePortal object.
        intent.setFlags(Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT | Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    @SuppressLint("NewApi")
    @CalledByNative
    private static void launch() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return;
        try {
            Context context = ContextUtils.getApplicationContext();
            ConnectivityManager connectivityManager =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            Network network = connectivityManager.getActiveNetwork();
            if (network == null) return;

            launchSignInApp(context, network);
        } catch (Exception e) {
            // Avoid crashing.
            Log.e(TAG, "Failed to launch sign-in app", e);
        }
    }
}
