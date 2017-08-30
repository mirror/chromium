// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ssl;

import android.content.Context;
import android.net.ConnectivityManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.lang.reflect.Method;

/** Helper class for captive portal related methods on Android. */
@JNINamespace("chrome::android")
public class CaptivePortalHelper {
    public static void addCaptivePortalCertificateForTesting(String spkiHash) {
        nativeAddCaptivePortalCertificateForTesting(spkiHash);
    }

    @CalledByNative
    private static String getCaptivePortalServerUrl() {
        try {
            Context context = ContextUtils.getApplicationContext();
            ConnectivityManager connectivityManager =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            Method getCaptivePortalServerUrlMethod =
                    connectivityManager.getClass().getMethod("getCaptivePortalServerUrl");
            return (String) getCaptivePortalServerUrlMethod.invoke(connectivityManager);
        } catch (Exception e) {
            // To avoid crashing, return the default portal check URL on Android.
            return "http://connectivitycheck.gstatic.com/generate_204";
        }
    }

    private CaptivePortalHelper() {}

    private static native void nativeAddCaptivePortalCertificateForTesting(String spkiHash);
}
