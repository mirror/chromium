// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.net.Uri;
import android.os.SystemClock;
import android.support.customtabs.CustomTabsIntent;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider;
import org.chromium.chrome.browser.externalnav.ExternalNavigationParams;
import org.chromium.chrome.browser.tab.InterceptNavigationDelegateImpl;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabRedirectHandler;
import org.chromium.chrome.browser.util.UrlUtilities;
import org.chromium.components.navigation_interception.NavigationParams;

import java.util.concurrent.TimeUnit;

/**
 * Intercepts navigations made by the Web App and sends off-origin http(s) ones to a Custom Tab.
 */
public class WebappInterceptNavigationDelegate extends InterceptNavigationDelegateImpl {
    static class CustomTabTimeSpendLogger {
        private static long sStartTime;
        private static @WebappActivity.ActivityType int sActivityType;

        static void startTime(@WebappActivity.ActivityType int activityType) {
            if (activityType == WebappActivity.ACTIVITY_TYPE_WEBAPP
                    || activityType == WebappActivity.ACTIVITY_TYPE_WEBAPK
                    || activityType == WebappActivity.ACTIVITY_TYPE_TRUSTED_WEB_ACTIVITY) {
                sActivityType = activityType;
                sStartTime = SystemClock.elapsedRealtime();
            }
        }

        static void stopTime() {
            long timeSpend = SystemClock.elapsedRealtime() - sStartTime;
            if (sActivityType == WebappActivity.ACTIVITY_TYPE_WEBAPP) {
                RecordHistogram.recordTimesHistogram(
                        "Webapp.CustomTab.Duration", timeSpend, TimeUnit.MILLISECONDS);
            }
            if (sActivityType == WebappActivity.ACTIVITY_TYPE_WEBAPK) {
                RecordHistogram.recordTimesHistogram(
                        "WebApk.CustomTab.Duration", timeSpend, TimeUnit.MILLISECONDS);
            }
            if (sActivityType == WebappActivity.ACTIVITY_TYPE_TRUSTED_WEB_ACTIVITY) {
                RecordHistogram.recordTimesHistogram(
                        "TrustedWebActivity.CustomTab.Duration", timeSpend, TimeUnit.MILLISECONDS);
            }
        }
    }

    private final WebappActivity mActivity;

    public static void startCountCustomTabTimeSpendIfNecessary(
            @WebappActivity.ActivityType int activityType) {
        CustomTabTimeSpendLogger.startTime(activityType);
    }

    public static void stopCountCustomTabTimeSpendIfNecessary() {
        CustomTabTimeSpendLogger.stopTime();
    }

    public WebappInterceptNavigationDelegate(WebappActivity activity, Tab tab) {
        super(tab);
        this.mActivity = activity;
    }

    @Override
    public boolean shouldIgnoreNavigation(NavigationParams navigationParams) {
        if (super.shouldIgnoreNavigation(navigationParams)) {
            return true;
        }

        if (shouldOpenInCustomTab(
                    navigationParams, mActivity.getWebappInfo(), mActivity.scopePolicy())) {
            CustomTabsIntent.Builder intentBuilder = new CustomTabsIntent.Builder();
            intentBuilder.setShowTitle(true);
            if (mActivity.mWebappInfo.hasValidThemeColor()) {
                // Need to cast as themeColor is a long to contain possible error results.
                intentBuilder.setToolbarColor((int) mActivity.mWebappInfo.themeColor());
            }
            CustomTabsIntent customTabIntent = intentBuilder.build();
            customTabIntent.intent.setPackage(mActivity.getPackageName());
            customTabIntent.intent.putExtra(
                    CustomTabIntentDataProvider.EXTRA_SEND_TO_EXTERNAL_DEFAULT_HANDLER, true);
            customTabIntent.intent.putExtra(CustomTabIntentDataProvider.EXTRA_BROWSER_LAUNCH_SOURCE,
                    mActivity.getActivityType());
            customTabIntent.launchUrl(mActivity, Uri.parse(navigationParams.url));
            return true;
        }

        return false;
    }

    @Override
    public ExternalNavigationParams.Builder buildExternalNavigationParams(
            NavigationParams navigationParams, TabRedirectHandler tabRedirectHandler,
            boolean shouldCloseTab) {
        ExternalNavigationParams.Builder builder = super.buildExternalNavigationParams(
                navigationParams, tabRedirectHandler, shouldCloseTab);
        builder.setNativeClientPackageName(mActivity.getNativeClientPackageName());
        return builder;
    }

    static boolean shouldOpenInCustomTab(
            NavigationParams navigationParams, WebappInfo info, WebappScopePolicy scopePolicy) {
        return UrlUtilities.isValidForIntentFallbackNavigation(navigationParams.url)
                && !navigationParams.isPost && !scopePolicy.isUrlInScope(info, navigationParams.url)
                && scopePolicy.openOffScopeNavsInCct();
    }
}
