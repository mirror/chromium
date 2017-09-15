// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;

import org.chromium.testing.local.LocalRobolectricTestRunner;
import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;
import org.chromium.webapk.test.WebApkTestHelper;

/** Unit tests for {@link MainActivity}. */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE, packageName = WebApkUtilsTest.WEBAPK_PACKAGE_NAME)
public final class MainActivityTest {
    /**
     * Test that MainActivity ignores the intent URL and uses the manifest start URL if scheme does
     * not match scope url and |loggedIntentUrlParam| is not set.
     */
    @Test
    public void testIgnoreIntentUrlOutOfScopeBecauseOfScheme() {
        final String intentStartUrl = "http://www.google.com/search_results?q=eh#cr=countryCA";
        final String manifestStartUrl = "https://www.google.com/index.html";
        final String manifestScope = "https://www.google.com/";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class).withIntent(launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(MainActivity.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                manifestStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Test that MainActivity ignores the intent URL and uses the manifest start URL if its path is
     * outside of the scope specified in the Android Manifest, and |loggedIntentUrlParam| is not
     * set.
     */
    @Test
    public void testIgnoreIntentUrlOutOfScopeBecauseOfPath() {
        final String intentStartUrl = "https://www.google.com/maps/";
        final String manifestStartUrl = "https://www.google.com/maps/contrib/startUrl";
        final String manifestScope = "https://www.google.com/maps/contrib/";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class).withIntent(launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(MainActivity.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                manifestStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Test that MainActivity rewrites the intent URL to the start URL with a custom query parameter
     * if:
     * 1) the intent URL is outside of the scope specified in the Android Manifest and
     * 2) |loggedIntentUrlParam| is specified in the Android Manifest.
     */
    @Test
    public void testRewriteStartUrlOutsideScopeAndAppendIntentUrl() {
        final String intentStartUrl = "https://maps.google.com/search_results?A=a";
        final String manifestStartUrl = "https://www.google.com/maps/startUrl";
        final String manifestScope = "https://www.google.com/maps";
        final String expectedRewrittenStartUrl =
                "https://www.google.com/maps/startUrl?originalUrl=https%253A%252F%252Fmaps.google.com%252Fsearch_results%253FA%253Da";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class).withIntent(launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(MainActivity.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(expectedRewrittenStartUrl,
                startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Test that the intent URL is not rewritten if it is inside the scope specified in the Android
     * Manifest.
     */
    @Test
    public void testNotRewriteStartUrlInsideScope() {
        final String intentStartUrl = "https://www.google.com/maps/address?A=a";
        final String manifestStartUrl = "https://www.google.com/maps/startUrl";
        final String manifestScope = "https://www.google.com/maps";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class).withIntent(launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(MainActivity.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                intentStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Test that MainActivity rewrites the start URL host so that it matches exactly the scope URL
     * host. In particular, MainActivity should not escape unicode characters.
     */
    @Test
    public void testRewriteUnicodeHost() {
        final String intentStartUrl = "https://www.google.com/";
        final String expectedStartUrl = "https://www.☺.com/";
        final String manifestStartUrl = "https://www.☺.com/";
        final String scope = "https://www.☺.com/";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, scope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class).withIntent(launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(MainActivity.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                expectedStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    private void installBrowser(String browserPackageName) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://"));
        RuntimeEnvironment.getRobolectricPackageManager().addResolveInfoForIntent(
                intent, newResolveInfo(browserPackageName));
    }

    private static ResolveInfo newResolveInfo(String packageName) {
        ActivityInfo activityInfo = new ActivityInfo();
        activityInfo.packageName = packageName;
        ResolveInfo resolveInfo = new ResolveInfo();
        resolveInfo.activityInfo = activityInfo;
        return resolveInfo;
    }
}
