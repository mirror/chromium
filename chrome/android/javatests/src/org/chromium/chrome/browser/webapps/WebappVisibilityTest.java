// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.support.test.filters.MediumTest;
import android.support.test.rule.UiThreadTestRule;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.blink_public.platform.WebDisplayMode;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.content.browser.test.NativeLibraryTestRule;

/**
 * Tests for {@link WebappDelegateFactory}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class WebappVisibilityTest {
    @Rule
    public NativeLibraryTestRule mActivityTestRule = new NativeLibraryTestRule();

    @Rule
    public UiThreadTestRule mUiThreadTestRule = new UiThreadTestRule();

    private static final String WEBAPP_URL = "http://originalwebsite.com";

    private static enum Type {
        WEBAPP,
        WEBAPK,
    }

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.loadNativeLibraryNoBrowserProcess();
    }

    @Test
    @MediumTest
    @Feature({"Webapps"})
    public void testShouldShowBrowserControls() throws Throwable {
        mUiThreadTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Type[] types = new Type[] {Type.WEBAPP, Type.WEBAPK};
                for (Type type : types) {
                    boolean isWebApk = (type == Type.WEBAPK);

                    // Show browser controls for out-of-domain URLs.
                    Assert.assertTrue(
                            shouldShowBrowserControls(WEBAPP_URL, "http://notoriginalwebsite.com",
                                    ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));
                    Assert.assertTrue(
                            shouldShowBrowserControls(WEBAPP_URL, "http://otherwebsite.com",
                                    ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));

                    // Do not show browser controls for subpaths.
                    Assert.assertFalse(shouldShowBrowserControls(WEBAPP_URL, WEBAPP_URL,
                            ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));
                    Assert.assertFalse(
                            shouldShowBrowserControls(WEBAPP_URL, WEBAPP_URL + "/things.html",
                                    ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));
                    Assert.assertFalse(
                            shouldShowBrowserControls(WEBAPP_URL, WEBAPP_URL + "/stuff.html",
                                    ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));

                    // For WebAPKs but not Webapps show browser controls for subdomains and private
                    // registries that are secure.
                    Assert.assertEquals(isWebApk,
                            shouldShowBrowserControls(WEBAPP_URL, "http://sub.originalwebsite.com",
                                    ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));
                    Assert.assertEquals(isWebApk,
                            shouldShowBrowserControls(WEBAPP_URL,
                                    "http://thing.originalwebsite.com",
                                    ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));

                    // Do not show browser controls when URL is not available yet.
                    Assert.assertFalse(shouldShowBrowserControls(WEBAPP_URL, "",
                            ConnectionSecurityLevel.NONE, type, WebDisplayMode.STANDALONE));

                    // Show browser controls for non secure URLs.
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL, WEBAPP_URL,
                            ConnectionSecurityLevel.SECURITY_WARNING, type,
                            WebDisplayMode.STANDALONE));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL,
                            WEBAPP_URL + "/things.html", ConnectionSecurityLevel.SECURITY_WARNING,
                            type, WebDisplayMode.STANDALONE));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL,
                            WEBAPP_URL + "/stuff.html", ConnectionSecurityLevel.SECURITY_WARNING,
                            type, WebDisplayMode.STANDALONE));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL,
                            WEBAPP_URL + "/stuff.html", ConnectionSecurityLevel.DANGEROUS, type,
                            WebDisplayMode.STANDALONE));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL,
                            WEBAPP_URL + "/things.html", ConnectionSecurityLevel.DANGEROUS, type,
                            WebDisplayMode.STANDALONE));
                    Assert.assertTrue(
                            shouldShowBrowserControls(WEBAPP_URL, "http://sub.originalwebsite.com",
                                    ConnectionSecurityLevel.SECURITY_WARNING, type,
                                    WebDisplayMode.STANDALONE));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL,
                            "http://notoriginalwebsite.com", ConnectionSecurityLevel.DANGEROUS,
                            type, WebDisplayMode.STANDALONE));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL,
                            "http://otherwebsite.com", ConnectionSecurityLevel.DANGEROUS, type,
                            WebDisplayMode.STANDALONE));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL,
                            "http://thing.originalwebsite.com", ConnectionSecurityLevel.DANGEROUS,
                            type, WebDisplayMode.STANDALONE));

                    // Show browser controls for Minimal-UI, but not for Fullscreen.
                    Assert.assertFalse(shouldShowBrowserControls(WEBAPP_URL, WEBAPP_URL,
                            ConnectionSecurityLevel.NONE, type, WebDisplayMode.FULLSCREEN));
                    Assert.assertTrue(shouldShowBrowserControls(WEBAPP_URL, WEBAPP_URL,
                            ConnectionSecurityLevel.NONE, type, WebDisplayMode.MINIMAL_UI));

                    // Allow auto-hiding controls unless we're on a dangerous connection.
                    Assert.assertTrue(
                            canAutoHideBrowserControls(type, ConnectionSecurityLevel.NONE));
                    Assert.assertTrue(
                            canAutoHideBrowserControls(type, ConnectionSecurityLevel.SECURE));
                    Assert.assertTrue(
                            canAutoHideBrowserControls(type, ConnectionSecurityLevel.EV_SECURE));
                    Assert.assertTrue(canAutoHideBrowserControls(
                            type, ConnectionSecurityLevel.HTTP_SHOW_WARNING));
                    Assert.assertFalse(
                            canAutoHideBrowserControls(type, ConnectionSecurityLevel.DANGEROUS));
                    Assert.assertFalse(canAutoHideBrowserControls(
                            type, ConnectionSecurityLevel.SECURITY_WARNING));
                }
            }
        });
    }

    /**
     * Calls either WebappBrowserControlsDelegate#shouldShowBrowserControls() or
     * WebApkBrowserControlsDelegate#shouldShowBrowserControls() based on the type.
     * @param webappStartUrlOrScopeUrl Web Manifest start URL when {@link type} == Type.WEBAPP and
     *                                 the Web Manifest scope URL otherwise.
     * @param url The current page URL
     * @param type
     */
    private static boolean shouldShowBrowserControls(String webappStartUrlOrScopeUrl, String url,
            int securityLevel, Type type, int displayMode) {
        return createDelegate(type).shouldShowBrowserControls(
                createWebappInfo(webappStartUrlOrScopeUrl, type, displayMode), url, securityLevel);
    }

    private static boolean canAutoHideBrowserControls(Type type, int securityLevel) {
        return createDelegate(type).canAutoHideBrowserControls(securityLevel);
    }

    private static WebappBrowserControlsDelegate createDelegate(Type type) {
        return type == Type.WEBAPP
                ? new WebappBrowserControlsDelegate(null, new Tab(0, false, null))
                : new WebApkBrowserControlsDelegate(null, new Tab(0, false, null));
    }

    private static WebappInfo createWebappInfo(
            String webappStartUrlOrScopeUrl, Type type, int displayMode) {
        return type == Type.WEBAPP
                ? WebappInfo.create("", webappStartUrlOrScopeUrl, null, null, null, null,
                          displayMode, 0, 0, 0, 0, false /* isIconGenerated */,
                          false /* forceNavigation */)
                : WebApkInfo.create("", "", webappStartUrlOrScopeUrl, null, null, null, null,
                          displayMode, 0, 0, 0, 0, "", 0, null, "", null,
                          false /* forceNavigation */);
    }
}
