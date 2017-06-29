// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import org.chromium.components.safe_browsing.SBThreatType;

/**
 * This is a helper class to map native SafeBrowsingActions and SAFE_BROWSING_THREATs to the
 * constants in WebViewClient.
 */
public final class AwSafeBrowsingConversionHelper {
    // TODO(ntfschr): set these values from WebViewClient after next SDK rolls
    /** The resource was blocked for an unknown reason */
    public static final int SAFE_BROWSING_THREAT_UNKNOWN = 0;
    /** The resource was blocked because it contains malware */
    public static final int SAFE_BROWSING_THREAT_MALWARE = 1;
    /** The resource was blocked because it contains deceptive content */
    public static final int SAFE_BROWSING_THREAT_PHISHING = 2;
    /** The resource was blocked because it contains unwanted software */
    public static final int SAFE_BROWSING_THREAT_UNWANTED_SOFTWARE = 3;

    /** Display the default interstitial */
    public static final int SAFE_BROWSING_ACTION_SHOW_INTERSTITIAL = 0;
    /** Act as if the user clicked "visit this unsafe site" */
    public static final int SAFE_BROWSING_ACTION_PROCEED = 1;
    /** Act as if the user clicked "Back to safety" */
    public static final int SAFE_BROWSING_ACTION_BACK_TO_SAFETY = 2;
    /** Display the default interstitial, disallow reporting checkbox */
    public static final int SAFE_BROWSING_ACTION_SHOW_INTERSTITIAL_NO_REPORTING = 3;
    /** Act as if the user clicked "visit this unsafe site" and send an extended report */
    public static final int SAFE_BROWSING_ACTION_PROCEED_AND_REPORT = 4;
    /** Act as if the user clicked "Back to safety" and send an extended report */
    public static final int SAFE_BROWSING_ACTION_BACK_TO_SAFETY_AND_REPORT = 5;

    /**
     * Converts the threat type value from SafeBrowsing code to the WebViewClient constant.
     */
    public static int convertThreatType(int chromiumThreatType) {
        switch (chromiumThreatType) {
            case SBThreatType.URL_MALWARE:
                return SAFE_BROWSING_THREAT_MALWARE;
            case SBThreatType.URL_PHISHING:
                return SAFE_BROWSING_THREAT_PHISHING;
            case SBThreatType.URL_UNWANTED:
                return SAFE_BROWSING_THREAT_UNWANTED_SOFTWARE;
            default:
                return SAFE_BROWSING_THREAT_UNKNOWN;
        }
    }

    /**
     * Converts the SAFE_BROWSING_ACTION_* received from the application to the SafeBrowsingAction
     * enum in AwSafeBrowsingResourceThrottle.
     */
    public static int convertAction(int appAction) {
        switch (appAction) {
            case SAFE_BROWSING_ACTION_SHOW_INTERSTITIAL:
                return SafeBrowsingAction.SHOW_INTERSTITIAL;
            case SAFE_BROWSING_ACTION_PROCEED:
                return SafeBrowsingAction.PROCEED;
            case SAFE_BROWSING_ACTION_BACK_TO_SAFETY:
                return SafeBrowsingAction.BACK_TO_SAFETY;
            case SAFE_BROWSING_ACTION_SHOW_INTERSTITIAL_NO_REPORTING:
                return SafeBrowsingAction.SHOW_INTERSTITIAL_NO_REPORTING;
            case SAFE_BROWSING_ACTION_PROCEED_AND_REPORT:
                return SafeBrowsingAction.PROCEED_AND_REPORT;
            case SAFE_BROWSING_ACTION_BACK_TO_SAFETY_AND_REPORT:
                return SafeBrowsingAction.BACK_TO_SAFETY_AND_REPORT;
            default:
                throw new RuntimeException("Invalid Safe Browsing action");
        }
    }

    // Do not instantiate this class.
    private AwSafeBrowsingConversionHelper() {}
}
