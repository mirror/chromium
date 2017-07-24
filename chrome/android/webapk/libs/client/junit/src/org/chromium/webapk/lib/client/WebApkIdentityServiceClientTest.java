// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.client;

import android.content.ComponentName;
import android.os.Bundle;
import android.os.RemoteException;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.testing.local.CustomShadowAsyncTask;
import org.chromium.testing.local.LocalRobolectricTestRunner;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;
import org.chromium.webapk.lib.common.identity_service.IIdentityService;
import org.chromium.webapk.test.WebApkTestHelper;

/**
 * Unit tests for {@link org.chromium.webapk.lib.client.WebApkIdentityServiceClient}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE, packageName = WebApkIdentityServiceClientTest.BROWSER_PACKAGE_NAME,
        shadows = {CustomShadowAsyncTask.class})
public class WebApkIdentityServiceClientTest {
    static final String BROWSER_PACKAGE_NAME = "browser";

    private static final String WEBAPK_PACKAGE_NAME = "org.chromium.webapk.test_package";
    private static final String ANOTHER_BROWSER_PACKAGE_NAME = "another.browser";
    private ShadowApplication mShadowApplication;

    /** Mocks the {@link WebApkIdentityServiceClient.CheckBrowserBacksWebApkCallback}. */
    private static class TestCheckBacksWebApkCallback
            implements WebApkIdentityServiceClient.CheckBrowserBacksWebApkCallback {
        private boolean mResult;
        private boolean mIsCalled;

        @Override
        public void onChecked(boolean doesBrowserBackWebApk) {
            mResult = doesBrowserBackWebApk;
            mIsCalled = true;
        }
    }

    /** Mocks the Identity service of a WebAPK. */
    private static class TestIdentityService extends IIdentityService.Stub {
        private String mRuntimeHost;

        public TestIdentityService(String runtimeHost) {
            mRuntimeHost = runtimeHost;
        }

        @Override
        public String getRuntimeHostBrowserPackageName() throws RemoteException {
            return mRuntimeHost;
        }
    }

    @Before
    public void setUp() {
        mShadowApplication = Shadows.shadowOf(RuntimeEnvironment.application);
        WebApkIdentityServiceClient.disconnectAll(RuntimeEnvironment.application);
    }

    /**
     * Tests that for WebAPKs with shell APK version lower than the
     * {@link WebApkIdentityServiceClient#SHELL_APK_VERSION_SUPPORTING_SWITCH_RUNTIME_HOST},
     * the backs-WebAPK-check returns false if the browser does NOT match the WebAPK's runtime host
     * specified in the metaData.
     */
    @Test
    public void testReturnsFalseWhenNotMatchRuntimeHostBeforeIntroduceHostBrowserSwitchLogic() {
        String webApkPackageName = WEBAPK_PACKAGE_NAME;
        String webApkRuntimeHost = ANOTHER_BROWSER_PACKAGE_NAME;
        int shellApkVersion =
                WebApkIdentityServiceClient.SHELL_APK_VERSION_SUPPORTING_SWITCH_RUNTIME_HOST - 1;
        registerWebApk(webApkPackageName, webApkRuntimeHost, shellApkVersion);

        checkBacksWebApk(webApkPackageName, false);
    }

    /**
     * Tests that for WebAPKs with shell APK version lower than the
     * {@link WebApkIdentityServiceClient#SHELL_APK_VERSION_SUPPORTING_SWITCH_RUNTIME_HOST},
     * the backs-WebAPK-check returns true if the browser matches the WebAPK's runtime host
     * specified in the metaData.
     */
    @Test
    public void testReturnsTrueWhenMatchesRuntimeHostBeforeIntroduceHostBrowserSwitchLogic() {
        String webApkPackageName = WEBAPK_PACKAGE_NAME;
        String webApkRuntimeHost = BROWSER_PACKAGE_NAME;
        int shellApkVersion =
                WebApkIdentityServiceClient.SHELL_APK_VERSION_SUPPORTING_SWITCH_RUNTIME_HOST - 1;
        registerWebApk(webApkPackageName, webApkRuntimeHost, shellApkVersion);

        checkBacksWebApk(webApkPackageName, true);
    }

    /**
     * Tests that for WebAPKs with shell APK version equal or higher than the
     * {@link WebApkIdentityServiceClient#SHELL_APK_VERSION_SUPPORTING_SWITCH_RUNTIME_HOST} but
     * doesn't have Identity Service, the backs-WebAPK-check returns false.
     */
    @Test
    public void testBacksWebApkCheckForWebApkWithHostBrowserSwitchLogicButWithoutIdentityService() {
        String webApkPackageName = WEBAPK_PACKAGE_NAME;
        String webApkRuntimeHost = BROWSER_PACKAGE_NAME;
        int shellApkVersion =
                WebApkIdentityServiceClient.SHELL_APK_VERSION_SUPPORTING_SWITCH_RUNTIME_HOST;
        registerWebApk(webApkPackageName, webApkRuntimeHost, shellApkVersion);

        checkBacksWebApk(webApkPackageName, false);
    }

    /**
     * Tests that for WebAPKs with Identity service, the backs-WebAPK-check returns false if the
     * package name of the browser does NOT match the one provided by the Identity service.
     */
    @Test
    public void testReturnsFalseWhenDoesNotMatchRuntimeHostProvidedByIdentityService() {
        String webApkPackageName = WEBAPK_PACKAGE_NAME;
        String webApkSpecifiedRuntimeHost = BROWSER_PACKAGE_NAME;
        String realRuntimeHost = ANOTHER_BROWSER_PACKAGE_NAME;
        // The shell APK version doesn't matter as long as the WebAPK has an Identity service.
        registerWebApk(webApkPackageName, webApkSpecifiedRuntimeHost, 0 /*shellApkVersion*/);
        mShadowApplication.setComponentNameAndServiceForBindService(
                new ComponentName(webApkPackageName, ""), new TestIdentityService(realRuntimeHost));

        checkBacksWebApk(webApkPackageName, false);
    }

    /**
     * Tests that for WebAPKs with Identity service, the backs-WebAPK-check returns true if the
     * package name of the browser matches the one provided by the Identity service.
     */
    @Test
    public void testReturnsTrueWhenMatchesRuntimeHostProvidedByIdentityService() {
        String webApkPackageName = WEBAPK_PACKAGE_NAME;
        String webApkSpecifiedRuntimeHost = ANOTHER_BROWSER_PACKAGE_NAME;
        String realRuntimeHost = BROWSER_PACKAGE_NAME;
        // The shell APK version doesn't matter as long as the WebAPK has an Identity service.
        registerWebApk(webApkPackageName, webApkSpecifiedRuntimeHost, 0 /*shellApkVersion*/);
        mShadowApplication.setComponentNameAndServiceForBindService(
                new ComponentName(webApkPackageName, ""), new TestIdentityService(realRuntimeHost));

        checkBacksWebApk(webApkPackageName, true);
    }

    /** Registers a WebAPK with the runtime host and the shell APK version in its metadata. */
    private void registerWebApk(String webApkPackageName, String runtimeHost, int shellApkVersion) {
        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, runtimeHost);
        bundle.putInt(WebApkMetaDataKeys.SHELL_APK_VERSION, shellApkVersion);
        WebApkTestHelper.registerWebApkWithMetaData(webApkPackageName, bundle);
    }

    /** Checks whether backs-WebAPK-check matches the expected result. */
    private void checkBacksWebApk(String webApkPackageName, final boolean expectedResult) {
        TestCheckBacksWebApkCallback callback = new TestCheckBacksWebApkCallback();
        WebApkIdentityServiceClient.getInstance().checkBrowserBacksWebApkAsync(
                RuntimeEnvironment.application, webApkPackageName, callback);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();

        Assert.assertTrue(callback.mIsCalled);
        Assert.assertEquals(expectedResult, callback.mResult);
    }
}
