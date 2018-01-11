// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.webkit.WebViewClient;

import org.chromium.components.safe_browsing.SBThreatType;

/**
 * This is a helper class to map native SafeBrowsingActions and SAFE_BROWSING_THREATs to the
 * constants in WebViewClient.
 */
public final class AwSafeBrowsingConversionHelper {
    /**
     * Converts the threat type value from SafeBrowsing code to the WebViewClient constant.
     */
    public static int convertThreatType(int chromiumThreatType) {
        switch (chromiumThreatType) {
            case SBThreatType.URL_MALWARE:
                return WebViewClient.SAFE_BROWSING_THREAT_MALWARE;
            case SBThreatType.URL_PHISHING:
                return WebViewClient.SAFE_BROWSING_THREAT_PHISHING;
            case SBThreatType.URL_UNWANTED:
                return WebViewClient.SAFE_BROWSING_THREAT_UNWANTED_SOFTWARE;
            default:
                return WebViewClient.SAFE_BROWSING_THREAT_UNKNOWN;
        }
    }

    // Do not instantiate this class.
    private AwSafeBrowsingConversionHelper() {}
}
